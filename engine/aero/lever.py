from ..contracts import Node,Binding
from . import DISTANCE,POSITION
NODE=Node('lever','CP−CG mesafesi',DISTANCE,{'cp':Binding('cp',POSITION),'cg':Binding('cg',POSITION)},lambda v,k:k.aero_lever(v['cp'],v['cg']),implementation='native/aero/lever.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
