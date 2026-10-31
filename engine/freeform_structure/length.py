from ..contracts import Node,Binding,LENGTH
from ..freeform import POINTS
NODE=Node('length','Serbest kanat eksenel boyu',LENGTH,{'points':Binding('points',POINTS)},
 lambda v,k:k.freeform_structure_length(v['points'],len(v['points'])//2),implementation='native/freeform/length.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Freeform Fin Set) · FinPlanform::axialLength')
