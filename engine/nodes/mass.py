from ..contracts import Node, Binding, VOLUME, DENSITY, MASS, REFERENCE

NODE = Node('mass', 'Kütle', MASS,
    {'volume': Binding('volume', VOLUME), 'density': Binding('density', DENSITY)},
    lambda v, kernel: kernel.tube_mass(v['volume'], v['density']),
    implementation='native/tube/mass.cpp', reference=REFERENCE + 'BodyTube::rawMassProperties · mass')
