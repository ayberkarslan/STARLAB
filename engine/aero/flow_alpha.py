from ..contracts import Node,Binding
from . import ANGLE,SPEED
NODE=Node('flowAlpha','Akış hücum açısı',ANGLE,{'speed':Binding('speed',SPEED),'alpha':Binding('alpha',ANGLE)},lambda v,k:k.aero_flow_alpha(v['speed'],v['alpha']),implementation='native/aero/flow_alpha.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
