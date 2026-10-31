from ..contracts import Node,Binding
from ..static_aero import CNA
from . import PORTS

NODE=Node('cna','Burun CNa',CNA,{k:Binding(k,p) for k,p in PORTS.items()},
 lambda v,k:k.static_nose_cna(*[v[n] for n in PORTS]),
 implementation='native/static_aero/nose_cna.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Nose Cone) · CNa and override')
