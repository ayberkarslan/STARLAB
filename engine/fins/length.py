from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('length','Eksenel boy',LENGTH,{'root':Binding('root',LENGTH),'tip':Binding('tip',WALL),'sweep':Binding('sweep',SIGNED_LENGTH)},
    lambda v,kernel:kernel.fin_length(v['root'],v['tip'],v['sweep']),
    implementation='native/fins/length.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
