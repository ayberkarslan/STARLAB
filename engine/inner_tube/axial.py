from ..contracts import Binding, INERTIA, LENGTH, MASS, Node

NODE = Node('axial', 'Eksenel atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH), 'innerRadius': Binding('innerRadius', LENGTH)},
    lambda v, kernel: kernel.inner_tube_axial(v['mass'], v['radius'], v['innerRadius']),
    implementation='native/inner_tube/axial.cpp',
    reference='RoketSiM src/components/InnerTube.cpp: InnerTube::rawMassProperties')
