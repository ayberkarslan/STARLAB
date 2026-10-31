from ..contracts import Node,Binding
from ..static_aero import CNA
from . import PORTS
NODE=Node('cna','Geçiş CNa',CNA,{k:Binding(k,p) for k,p in PORTS.items()},lambda v,k:k.static_transition_cna(*[v[n] for n in PORTS]),implementation='native/static_aero/transition_cna.cpp',reference='RoketSiM include/aero/Barrowman.hpp::transitionCna + override')
