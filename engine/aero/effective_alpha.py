from ..contracts import Node,Binding
from . import ANGLE
NODE=Node('effectiveAlpha','Sınırlı hücum açısı',ANGLE,{'alpha':Binding('flowAlpha',ANGLE)},lambda v,k:k.aero_effective_alpha(v['alpha']),implementation='native/aero/effective_alpha.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
