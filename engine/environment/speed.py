from ..contracts import Node,Binding
from . import SPEED,VELOCITY
NODE=Node('speed','Havaya göre hız',SPEED,{'horizontal':Binding('airDownrange',VELOCITY),'vertical':Binding('groundVertical',VELOCITY)},lambda v,k:k.environment_speed(v['horizontal'],v['vertical']),implementation='native/environment/speed.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv')
