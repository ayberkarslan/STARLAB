from ..contracts import Node,Binding,MASS,Port
from ..motor import CURVE
from . import SERIES

NONNEG=Port('nonnegative','1')
POSITIVE=Port('positive','1',positive=True)
SIGNED=Port('signed','1',signed=True)
ATMOSPHERE=Port('atmosphere-model','1',choices=(0.0,1.0))

def solve(v,k):
    output=[0.0]*20000
    rows=k.flight_axial_solve(v['curve'],len(v['curve'])//2,v['wetMass'],v['propellant'],v['cd'],v['area'],v['atmosphereModel'],v['launchAltitude'],v['surfaceTemperature'],v['surfacePressure'],v['windSpeed'],v['windDirection'],v['heading'],v['railAngle'],v['railLength'],v['dt'],v['maxTime'],output,len(output))
    count=int(rows)
    if count<2 or count*4>len(output):raise ValueError('Uçuş çözücüsü geçerli seri üretmedi.')
    return output[:count*4]

NODE=Node('series','RK4 uçuş serisi',SERIES,{
 'curve':Binding('curve',CURVE),'wetMass':Binding('wetMass',MASS),'propellant':Binding('propellant',MASS),
 'cd':Binding('cd',NONNEG),'area':Binding('area',POSITIVE),'atmosphereModel':Binding('atmosphereModel',ATMOSPHERE),
 'launchAltitude':Binding('launchAltitude',SIGNED),'surfaceTemperature':Binding('surfaceTemperature',POSITIVE),'surfacePressure':Binding('surfacePressure',POSITIVE),
 'windSpeed':Binding('windSpeed',NONNEG),'windDirection':Binding('windDirection',NONNEG),'heading':Binding('heading',NONNEG),'railAngle':Binding('railAngle',NONNEG),'railLength':Binding('railLength',POSITIVE),
 'dt':Binding('dt',POSITIVE),'maxTime':Binding('maxTime',POSITIVE),
},solve,version='1.1.0',implementation='native/flight/axial.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv/rk4Step')
