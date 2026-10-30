from ..contracts import Node,Binding
from . import EXPONENT,HEIGHT,MULTIPLIER,POSITIVE_HEIGHT,WIND_MODEL,profile_multiplier
NODE=Node('windMultiplier','Rüzgâr yükseklik çarpanı',MULTIPLIER,{'model':Binding('windModel',WIND_MODEL),'height':Binding('height',HEIGHT),'reference':Binding('referenceHeight',POSITIVE_HEIGHT),'minimum':Binding('minimumHeight',HEIGHT),'exponent':Binding('exponent',EXPONENT),'roughness':Binding('roughness',POSITIVE_HEIGHT)},profile_multiplier,implementation='native/environment/wind_multiplier.cpp',reference='RoketSiM src/env/WindModel.cpp · turbulence disabled')
