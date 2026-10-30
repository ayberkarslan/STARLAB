from ..contracts import Node,Binding
from . import ALTITUDE,HEIGHT
NODE=Node('absoluteAltitude','Deniz seviyesine göre irtifa',ALTITUDE,{'launchAltitude':Binding('launchAltitude',ALTITUDE),'height':Binding('height',HEIGHT)},lambda v,k:k.environment_absolute_altitude(v['launchAltitude'],v['height']),implementation='native/environment/absolute_altitude.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv')
