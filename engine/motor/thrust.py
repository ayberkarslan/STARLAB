from ..contracts import Node,Binding
from . import CURVE,TIME,FORCE
NODE=Node('thrust','Anlık itki',FORCE,{'curve':Binding('curve',CURVE),'time':Binding('time',TIME)},lambda v,k:k.motor_thrust(v['curve'],len(v['curve'])//2,v['time']),implementation='native/motor/thrust.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::lerp')
