from ..contracts import Node,Binding
from . import DIRECTION,SPEED,WIND_COMPONENT
NODE=Node('windNorth','Kuzey rüzgârı',WIND_COMPONENT,{'speed':Binding('windSpeed',SPEED),'direction':Binding('windDirection',DIRECTION)},lambda v,k:k.environment_wind_north(v['speed'],v['direction']),implementation='native/environment/wind_north.cpp',reference='RoketSiM src/env/WindModel.cpp · turbulence disabled')
