from ..contracts import Node, Binding, DENSITY, INERTIA
from . import PORTS, validate

def compute(v,kernel):
    validate(v)
    return kernel.nose_axial(*[v[k] for k in PORTS],v['density'])

NODE=Node('axial','Eksenel atalet',INERTIA,{k:Binding(k,port) for k,port in {**PORTS, 'density': DENSITY}.items()},compute,
    implementation='native/nose/axial.cpp',reference='RoketSiM src/cli/design_analysis.hpp: noseRadius + integrateShell + analyzeComponent(Nose Cone)')
