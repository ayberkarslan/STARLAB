from ..contracts import Node,Binding
from . import CURVE,IMPULSE
NODE=Node('totalImpulse','Toplam impuls',IMPULSE,{'curve':Binding('curve',CURVE)},lambda v,k:k.motor_total_impulse(v['curve'],len(v['curve'])//2),implementation='native/motor/total_impulse.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::remainingPropellantKg · totalImpulse loop')
