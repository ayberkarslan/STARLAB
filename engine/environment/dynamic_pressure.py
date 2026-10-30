from ..contracts import Node,Binding
from . import DENSITY,DYNAMIC_PRESSURE,SPEED
NODE=Node('dynamicPressure','Dinamik basınç',DYNAMIC_PRESSURE,{'density':Binding('density',DENSITY),'speed':Binding('speed',SPEED)},lambda v,k:k.environment_dynamic_pressure(v['density'],v['speed']),implementation='native/environment/dynamic_pressure.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv')
