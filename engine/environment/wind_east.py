from ..contracts import Node,Binding
from . import DIRECTION,SPEED,WIND_COMPONENT
NODE=Node('windEast','Doğu rüzgârı',WIND_COMPONENT,{'speed':Binding('windSpeed',SPEED),'direction':Binding('windDirection',DIRECTION)},lambda v,k:k.environment_wind_east(v['speed'],v['direction']),implementation='native/environment/wind_east.cpp',reference='RoketSiM src/env/WindModel.cpp · turbulence disabled')
