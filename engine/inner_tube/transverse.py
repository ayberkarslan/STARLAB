from ..contracts import Binding, INERTIA, LENGTH, MASS, Node

NODE = Node('transverse', 'Enine atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH), 'innerRadius': Binding('innerRadius', LENGTH), 'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.inner_tube_transverse(v['mass'], v['radius'], v['innerRadius'], v['length']),
    implementation='native/inner_tube/transverse.cpp',
    reference='RoketSiM src/components/InnerTube.cpp: InnerTube::rawMassProperties')
