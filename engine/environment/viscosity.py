from ..contracts import Node,Binding
from . import TEMPERATURE,VISCOSITY
NODE=Node('viscosity','Dinamik viskozite',VISCOSITY,{'temperature':Binding('temperature',TEMPERATURE)},lambda v,k:k.environment_viscosity(v['temperature']),implementation='native/environment/viscosity.cpp',reference='RoketSiM src/env/Atmosphere.cpp')
