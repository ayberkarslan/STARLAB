from ..contracts import Node,Binding
from . import ALTITUDE,GRAVITY
NODE=Node('gravity','Yerel yerçekimi',GRAVITY,{'altitude':Binding('absoluteAltitude',ALTITUDE)},lambda v,k:k.environment_gravity(v['altitude']),implementation='native/environment/gravity.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv')
