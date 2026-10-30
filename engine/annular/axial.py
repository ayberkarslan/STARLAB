from ..contracts import WALL, Binding, INERTIA, LENGTH, MASS, Node

NODE = Node('axial', 'Eksenel atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH), 'innerRadius': Binding('innerRadius', WALL)},
    lambda v, kernel: kernel.annular_axial(v['mass'], v['radius'], v['innerRadius']),
    implementation='native/annular/axial.cpp',
    reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Centering Ring / Bulkhead) · rawMassProperties')
