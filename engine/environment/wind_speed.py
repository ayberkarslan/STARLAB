from ..contracts import Node,Binding
from . import MULTIPLIER,SPEED
NODE=Node('windSpeed','Yerel rüzgâr hızı',SPEED,{'meanSpeed':Binding('meanSpeed',SPEED),'multiplier':Binding('windMultiplier',MULTIPLIER)},lambda v,k:k.environment_wind_speed(v['meanSpeed'],v['multiplier']),implementation='native/environment/wind_speed.cpp',reference='RoketSiM src/env/WindModel.cpp · turbulence disabled')
