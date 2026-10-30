from ..contracts import Binding, CG, LENGTH, Node

NODE = Node('cg', 'Yerel CG', CG,
    {'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.mass_component_cg(v['length']),
    implementation='native/mass_component/centroid.cpp',
    reference='RoketSiM src/components/MassComponent.cpp: MassComponent::rawMassProperties')
