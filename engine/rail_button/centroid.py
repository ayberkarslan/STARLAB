from ..contracts import Node,Binding,CG
from . import PORTS,validate
def compute(v,k):validate(v);return k.rail_button_cg(*[v[n] for n in PORTS])
NODE=Node('cg','Yerel grup CG',CG,{n:Binding(n,p) for n,p in PORTS.items()},compute,implementation='native/rail_button/centroid.cpp',reference='RoketSiM src/cli/design_analysis.hpp::aggregateLocalCg · height/2 + mean instance offset')
