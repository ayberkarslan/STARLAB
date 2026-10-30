from ..contracts import Node,Binding
from . import MACH,SOUND,SPEED
NODE=Node('mach','Mach',MACH,{'speed':Binding('speed',SPEED),'sound':Binding('speedOfSound',SOUND)},lambda v,k:k.environment_mach(v['speed'],v['sound']),implementation='native/environment/mach.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv')
