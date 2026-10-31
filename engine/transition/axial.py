from ..contracts import Node,Binding,DENSITY,INERTIA
from . import PORTS,validate
def compute(v,k):validate(v);return k.transition_axial(*[v[n] for n in PORTS],v['density'])
NODE=Node('axial','Eksenel atalet',INERTIA,{**{n:Binding(n,p) for n,p in PORTS.items()},'density':Binding('density',DENSITY)},compute,implementation='native/transition/axial.cpp',reference='RoketSiM src/components/Transition.cpp::rawMassProperties · Izz midpoint integration, 2000 slices')
