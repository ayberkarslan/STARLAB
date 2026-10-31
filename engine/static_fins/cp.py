from ..contracts import Node,Binding
from ..static_aero import CP
from . import PORTS

NODE=Node('cp','Kanat yerel CP',CP,{k:Binding(k,p) for k,p in PORTS.items()},
 lambda v,k:k.static_fin_cp(*[v[n] for n in PORTS]),
 implementation='native/static_aero/fin_cp.cpp',reference='RoketSiM include/aero/FinSetBarrowman.hpp::cpFraction + src/cli/design_analysis.hpp trapezoidal MAC · override')
