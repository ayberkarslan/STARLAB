from ..contracts import Node,Binding
from . import CURVE,TIME,IMPULSE
NODE=Node('impulse','Tüketilen impuls',IMPULSE,{'curve':Binding('curve',CURVE),'time':Binding('time',TIME)},lambda v,k:k.motor_impulse(v['curve'],len(v['curve'])//2,v['time']),implementation='native/motor/impulse.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::remainingPropellantKg · impulse loop')
