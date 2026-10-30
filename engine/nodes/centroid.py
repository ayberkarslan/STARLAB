from ..contracts import Node, Binding, LENGTH, CG, REFERENCE

NODE = Node('cg', 'Yerel CG', CG, {'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.tube_cg(v['length']),
    implementation='native/tube/centroid.cpp', reference=REFERENCE + 'BodyTube::rawMassProperties · cgLocal.z')
