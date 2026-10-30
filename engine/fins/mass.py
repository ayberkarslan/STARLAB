from ..contracts import Node, Binding, LENGTH, WALL, DENSITY, VOLUME, MASS, INERTIA
from . import COUNT,SECTION,SIGNED_LENGTH,AREA,FACTOR,SIGNED_CG

NODE=Node('mass','Kütle',MASS,{'volume':Binding('volume',VOLUME),'density':Binding('density',DENSITY)},
    lambda v,kernel:kernel.fin_mass(v['volume'],v['density']),
    implementation='native/fins/mass.cpp',reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Fins), trapezoidal branch')
