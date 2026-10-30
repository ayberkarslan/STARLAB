from ..contracts import Node,Binding
from . import SOUND,TEMPERATURE
NODE=Node('speedOfSound','Ses hızı',SOUND,{'temperature':Binding('temperature',TEMPERATURE)},lambda v,k:k.environment_speed_of_sound(v['temperature']),implementation='native/environment/speed_of_sound.cpp',reference='RoketSiM src/env/Atmosphere.cpp')
