from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('sectionFactor','Kesit katsayısı',FACTOR,{'section':Binding('sectionCode',SECTION)},
    lambda v,kernel:kernel.fin_section(v['section']),
    implementation='native/fins/section.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
