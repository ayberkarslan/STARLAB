from ..contracts import Node, Binding, DENSITY, CG
from . import PORTS, validate

def compute(v,kernel):
    validate(v)
    return kernel.nose_cg(*[v[k] for k in PORTS])

NODE=Node('cg','Yerel CG',CG,{k:Binding(k,port) for k,port in PORTS.items()},compute,
    implementation='native/nose/centroid.cpp',reference='RoketSiM src/cli/design_analysis.hpp: noseRadius + integrateShell + analyzeComponent(Nose Cone)')
