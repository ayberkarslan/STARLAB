from ..contracts import Node,Binding,MASS,Port

def compute(v,k):
    if v['propellant']>v['wet']:raise ValueError('Yakıt kütlesi toplam motor kütlesini aşamaz.')
    return k.motor_dry(v['wet'],v['propellant'])
NODE=Node('dry','Kuru kütle',MASS,{'wet':Binding('wet',Port('positive-motor-mass','kg',positive=True)),'propellant':Binding('propellant',MASS)},compute,implementation='native/motor/dry.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp · motor dry mass')
