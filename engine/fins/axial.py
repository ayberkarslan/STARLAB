from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('axial','Eksenel atalet',INERTIA,{'mass':Binding('mass',MASS),'radial':Binding('radial',WALL)},
    lambda v,kernel:kernel.fin_axial(v['mass'],v['radial']),
    implementation='native/fins/axial.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
