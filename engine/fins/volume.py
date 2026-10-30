from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('volume','Hacim',VOLUME,{'count':Binding('count',COUNT),'area':Binding('area',AREA),'tabHeight':Binding('tabHeight',WALL),'tabLength':Binding('tabLength',WALL),'thickness':Binding('thickness',LENGTH),'sectionFactor':Binding('sectionFactor',FACTOR)},
    lambda v,kernel:kernel.fin_volume(v['count'],v['area'],v['tabHeight'],v['tabLength'],v['thickness'],v['sectionFactor']),
    implementation='native/fins/volume.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
