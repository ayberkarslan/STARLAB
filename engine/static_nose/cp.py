from ..contracts import Node,Binding
from ..static_aero import CP
from . import PORTS

NODE=Node('cp','Burun yerel CP',CP,{k:Binding(k,p) for k,p in PORTS.items()},
 lambda v,k:k.static_nose_cp(*[v[n] for n in PORTS]),
 implementation='native/static_aero/nose_cp.cpp',reference='RoketSiM src/cli/design_analysis.hpp::tangentOgiveCpFromVolume + analyzeComponent(Nose Cone) · CP and override')
