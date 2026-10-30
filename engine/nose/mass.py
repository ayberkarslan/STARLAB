from ..contracts import Node, Binding, DENSITY, MASS
from . import PORTS, validate

def compute(v,kernel):
    validate(v)
    return kernel.nose_mass(*[v[k] for k in PORTS],v['density'])

NODE=Node('mass','Kütle',MASS,{k:Binding(k,port) for k,port in {**PORTS, 'density': DENSITY}.items()},compute,
    implementation='native/nose/mass.cpp',reference='RoketSiM src/cli/design_analysis.hpp: noseRadius + integrateShell + analyzeComponent(Nose Cone)')
