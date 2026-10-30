from ..contracts import Node, Binding, LENGTH, WALL, MASS, INERTIA, REFERENCE

NODE = Node('transverse', 'Enine atalet', INERTIA,
    {'mass': Binding('mass', MASS), 'radius': Binding('radius', LENGTH), 'wall': Binding('wall', WALL), 'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.tube_transverse(v['mass'], v['radius'], v['wall'], v['length']),
    implementation='native/tube/transverse.cpp', reference=REFERENCE + 'BodyTube::rawMassProperties · Ixx = Iyy → STARLAB Iyy = Izz')
