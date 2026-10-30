from ..contracts import Binding, CG, LENGTH, Node

NODE = Node('cg', 'Yerel CG', CG,
    {'length': Binding('length', LENGTH)},
    lambda v, kernel: kernel.annular_cg(v['length']),
    implementation='native/annular/centroid.cpp',
    reference='RoketSiM src/cli/design_analysis.hpp: analyzeComponent(Centering Ring / Bulkhead) · localCg')
