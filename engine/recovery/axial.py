from ..contracts import Node,Binding,MASS,INERTIA
NODE=Node('axial','Eksenel atalet',INERTIA,{'mass':Binding('mass',MASS)},lambda v,k:k.recovery_inertia(v['mass']),implementation='native/recovery/inertia.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Parachute / Shock Cord) · no inertia assigned')
