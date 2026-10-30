from ..contracts import Binding, MASS, Node

from ..contracts import Port
POSITIVE_MASS = Port('positive-mass', 'kg', positive=True)

NODE = Node('mass', 'Kütle', MASS,
    {'suppliedMass': Binding('suppliedMass', POSITIVE_MASS)},
    lambda v, kernel: kernel.mass_component_mass(v['suppliedMass']),
    implementation='native/mass_component/mass.cpp',
    reference='RoketSiM src/components/MassComponent.cpp: MassComponent::rawMassProperties')
