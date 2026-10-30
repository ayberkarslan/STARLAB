from ..contracts import Binding, DENSITY, MASS, Node, VOLUME

NODE = Node('mass', 'Kütle', MASS,
    {'volume': Binding('volume', VOLUME), 'density': Binding('density', DENSITY)},
    lambda v, kernel: kernel.annular_mass(v['volume'], v['density']),
    implementation='native/annular/mass.cpp',
    reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Centering Ring / Bulkhead) · mass')
