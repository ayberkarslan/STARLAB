from ..contracts import Node,Binding,VOLUME
from . import PORTS,validate
def compute(v,k):validate(v);return k.transition_volume(*[v[n] for n in PORTS])
NODE=Node('volume','Hacim',VOLUME,{n:Binding(n,p) for n,p in PORTS.items()},compute,implementation='native/transition/volume.cpp',reference='RoketSiM src/components/Transition.cpp::volume')
