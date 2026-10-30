from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('area','Alan',AREA,{'root':Binding('root',LENGTH),'tip':Binding('tip',WALL),'span':Binding('span',LENGTH)},
    lambda v,kernel:kernel.fin_area(v['root'],v['tip'],v['span']),
    implementation='native/fins/area.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
