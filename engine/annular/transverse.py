from ..contracts import WALL, Binding, INERTIA, LENGTH, MASS, Node

NODE = Node('transverse', 'Enine atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH), 'innerRadius': Binding('innerRadius', WALL), 'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.annular_transverse(v['mass'], v['radius'], v['innerRadius'], v['length']),
    implementation='native/annular/transverse.cpp',
    reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Centering Ring / Bulkhead) · rawMassProperties')
