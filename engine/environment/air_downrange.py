from ..contracts import Node,Binding
from . import VELOCITY,WIND_COMPONENT
NODE=Node('airDownrange','Havaya göre yatay hız',VELOCITY,{'ground':Binding('groundDownrange',VELOCITY),'wind':Binding('windAlong',WIND_COMPONENT)},lambda v,k:k.environment_air_downrange(v['ground'],v['wind']),implementation='native/environment/air_downrange.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv')
