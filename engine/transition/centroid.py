from ..contracts import Node,Binding,CG
from . import PORTS,validate
def compute(v,k):validate(v);return k.transition_cg(*[v[n] for n in PORTS])
NODE=Node('cg','Yerel CG',CG,{n:Binding(n,p) for n,p in PORTS.items()},compute,implementation='native/transition/centroid.cpp',reference='RoketSiM src/components/Transition.cpp::cgZ')
