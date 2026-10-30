from ..contracts import Binding, INERTIA, LENGTH, MASS, Node

NODE = Node('transverse', 'Enine atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH), 'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.mass_component_transverse(v['mass'], v['radius'], v['length']),
    implementation='native/mass_component/transverse.cpp',
    reference='RoketSiM src/components/MassComponent.cpp: MassComponent::rawMassProperties')
