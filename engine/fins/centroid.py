from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('cg','Yerel CG',SIGNED_CG,{'root':Binding('root',LENGTH),'tip':Binding('tip',WALL),'sweep':Binding('sweep',SIGNED_LENGTH)},
    lambda v,kernel:kernel.fin_cg(v['root'],v['tip'],v['sweep']),
    implementation='native/fins/centroid.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
