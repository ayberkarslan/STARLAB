from ..contracts import Binding, INERTIA, LENGTH, MASS, Node

NODE = Node('axial', 'Eksenel atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH)},
    lambda v, kernel: kernel.mass_component_axial(v['mass'], v['radius']),
    implementation='native/mass_component/axial.cpp',
    reference='RoketSiM src/components/MassComponent.cpp: MassComponent::rawMassProperties')
