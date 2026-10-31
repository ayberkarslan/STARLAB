from ..contracts import Node,Binding
from ..static_aero import CNA
from . import PORTS

NODE=Node('cna','Kanat CNa',CNA,{k:Binding(k,p) for k,p in PORTS.items()},
 lambda v,k:k.static_fin_cna(*[v[n] for n in PORTS]),
 implementation='native/static_aero/fin_cna.cpp',reference='RoketSiM include/aero/FinSetBarrowman.hpp::aggregateCna + src/cli/design_analysis.hpp::finCnaAtMach · override')
