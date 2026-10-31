from ..contracts import Node,Binding,LENGTH,CG
NODE=Node('cg','Yerel CG',CG,{'length':Binding('length',LENGTH)},lambda v,k:k.recovery_cg(v['length']),implementation='native/recovery/centroid.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Parachute / Shock Cord) · packed length / 2')
