from ..contracts import Node,Binding,LENGTH,CG
NODE=Node('cg','Motor yerel CG',CG,{'length':Binding('length',LENGTH)},lambda v,k:k.motor_cg(v['length']),implementation='native/motor/centroid.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Motor) · midpoint fallback')
