from ..contracts import Node,Binding,MASS
NODE=Node('mass','Paketlenmiş kütle',MASS,{'mass':Binding('suppliedMass',MASS)},lambda v,k:k.recovery_mass(v['mass']),implementation='native/recovery/mass.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Parachute / Shock Cord) · supplied packed mass')
