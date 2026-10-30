from ..contracts import Binding, DENSITY, MASS, Node, VOLUME

NODE = Node('mass', 'Kütle', MASS,
    {'volume': Binding('volume', VOLUME), 'density': Binding('density', DENSITY)},
    lambda v, kernel: kernel.inner_tube_mass(v['volume'], v['density']),
    implementation='native/inner_tube/mass.cpp',
    reference='RoketSiM src/components/InnerTube.cpp: InnerTube::rawMassProperties')
