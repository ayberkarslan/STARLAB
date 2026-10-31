from ..contracts import Node,Binding
from ..static_aero import CP
from . import PORTS
NODE=Node('cp','Geçiş yerel CP',CP,{k:Binding(k,p) for k,p in PORTS.items()},lambda v,k:k.static_transition_cp(*[v[n] for n in PORTS]),implementation='native/static_aero/transition_cp.cpp',reference='RoketSiM include/aero/Barrowman.hpp::transitionCpLocal + override')
