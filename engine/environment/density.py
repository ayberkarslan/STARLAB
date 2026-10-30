from ..contracts import Node,Binding
from . import DENSITY,PRESSURE,TEMPERATURE
NODE=Node('density','Hava yoğunluğu',DENSITY,{'pressure':Binding('pressure',PRESSURE),'temperature':Binding('temperature',TEMPERATURE)},lambda v,k:k.environment_density(v['pressure'],v['temperature']),implementation='native/environment/density.cpp',reference='RoketSiM src/env/Atmosphere.cpp')
