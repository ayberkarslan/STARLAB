from ..contracts import Node,Binding,LENGTH
from . import DENSITY,REYNOLDS,SPEED,VISCOSITY
NODE=Node('reynolds','Reynolds',REYNOLDS,{'speed':Binding('speed',SPEED),'length':Binding('referenceLength',LENGTH),'viscosity':Binding('viscosity',VISCOSITY),'density':Binding('density',DENSITY)},lambda v,k:k.environment_reynolds(v['speed'],v['length'],v['viscosity'],v['density']),implementation='native/environment/reynolds.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv')
