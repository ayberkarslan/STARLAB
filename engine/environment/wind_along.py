from ..contracts import Node,Binding
from . import DIRECTION,WIND_COMPONENT
NODE=Node('windAlong','Uçuş düzlemindeki rüzgâr',WIND_COMPONENT,{'east':Binding('windEast',WIND_COMPONENT),'north':Binding('windNorth',WIND_COMPONENT),'heading':Binding('heading',DIRECTION)},lambda v,k:k.environment_wind_along(v['east'],v['north'],v['heading']),implementation='native/environment/wind_along.cpp',reference='RoketSiM src/env/WindModel.cpp · turbulence disabled')
