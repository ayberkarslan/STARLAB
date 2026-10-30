from ..contracts import Node,Binding
from . import DENSITY,PRESSURE,SPEED
NODE=Node('dynamicPressure','Dinamik basınç',PRESSURE,{'density':Binding('density',DENSITY),'speed':Binding('speed',SPEED)},lambda v,k:k.aero_dynamic_pressure(v['density'],v['speed']),implementation='native/aero/dynamic_pressure.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
