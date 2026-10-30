from ..contracts import Node,Binding
from . import DIRECTION,GRADIENT,HEIGHT
NODE=Node('windDirection','Rüzgârın geldiği yön',DIRECTION,{'direction':Binding('direction',DIRECTION),'gradient':Binding('directionGradient',GRADIENT),'height':Binding('height',HEIGHT)},lambda v,k:k.environment_wind_direction(v['direction'],v['gradient'],v['height']),implementation='native/environment/wind_direction.cpp',reference='RoketSiM src/env/WindModel.cpp · turbulence disabled')
