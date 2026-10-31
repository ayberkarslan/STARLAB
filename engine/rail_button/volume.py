from ..contracts import Node,Binding,VOLUME
from . import PORTS,validate
def compute(v,k):validate(v);return k.rail_button_volume(*[v[n] for n in PORTS])
NODE=Node('volume','Hacim',VOLUME,{n:Binding(n,p) for n,p in PORTS.items()},compute,implementation='native/rail_button/volume.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Rail Button) · physical volume and instances')
