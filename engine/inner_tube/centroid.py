from ..contracts import Binding, CG, LENGTH, Node

NODE = Node('cg', 'Yerel CG', CG,
    {'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.inner_tube_cg(v['length']),
    implementation='native/inner_tube/centroid.cpp',
    reference='RoketSiM src/components/InnerTube.cpp: InnerTube::rawMassProperties')
