from ..contracts import Node,Binding
from ..freeform import POINTS
from ..fins import SIGNED_CG
NODE=Node('cg','Serbest kanat yerel CG',SIGNED_CG,{'points':Binding('points',POINTS)},
 lambda v,k:k.freeform_structure_centroid(v['points'],len(v['points'])//2),implementation='native/freeform/centroid.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Freeform Fin Set) · FinPlanform::centroidX')
