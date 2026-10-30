from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('radial','Radyal konum',WALL,{'bodyRadius':Binding('bodyRadius',WALL),'span':Binding('span',LENGTH)},
    lambda v,kernel:kernel.fin_radial(v['bodyRadius'],v['span']),
    implementation='native/fins/radial.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
