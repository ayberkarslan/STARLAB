from ..contracts import Node, Binding, DENSITY, VOLUME
from . import PORTS, validate

def compute(v,kernel):
    validate(v)
    return kernel.nose_volume(*[v[k] for k in PORTS])

NODE=Node('volume','Hacim',VOLUME,{k:Binding(k,port) for k,port in PORTS.items()},compute,
    implementation='native/nose/volume.cpp',reference='RoketSiM src/cli/design_analysis.hpp: noseRadius + integrateShell + analyzeComponent(Nose Cone)')
