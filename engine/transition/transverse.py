from ..contracts import Node,Binding,DENSITY,CG,INERTIA
from . import PORTS,validate
def compute(v,k):validate(v);return k.transition_transverse(*[v[n] for n in PORTS],v['density'],v['cg'])
NODE=Node('transverse','Enine atalet',INERTIA,{**{n:Binding(n,p) for n,p in PORTS.items()},'density':Binding('density',DENSITY),'cg':Binding('cg',CG)},compute,implementation='native/transition/transverse.cpp',reference='RoketSiM src/components/Transition.cpp::rawMassProperties · Ixx midpoint integration, 2000 slices')
