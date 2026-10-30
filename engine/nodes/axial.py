from ..contracts import Node, Binding, LENGTH, WALL, MASS, INERTIA, REFERENCE

NODE = Node('axial', 'Eksenel atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH), 'wall': Binding('wall', WALL)},
    lambda v, kernel: kernel.tube_axial(v['mass'], v['radius'], v['wall']),
    implementation='native/tube/axial.cpp', reference=REFERENCE + 'BodyTube::rawMassProperties · Izz → STARLAB Ixx')
