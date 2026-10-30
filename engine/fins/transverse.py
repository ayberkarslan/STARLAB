from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('transverse','Enine atalet',INERTIA,{'mass':Binding('mass',MASS),'length':Binding('length',LENGTH),'radial':Binding('radial',WALL)},
    lambda v,kernel:kernel.fin_transverse(v['mass'],v['length'],v['radial']),
    implementation='native/fins/transverse.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
