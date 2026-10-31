from ..contracts import Node,Binding,WALL
from ..freeform import POINTS
NODE=Node('radial','Serbest kanat radyal konumu',WALL,{'points':Binding('points',POINTS),'bodyRadius':Binding('bodyRadius',WALL)},
 lambda v,k:k.freeform_structure_radial(v['points'],len(v['points'])//2,v['bodyRadius']),implementation='native/freeform/radial.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Freeform Fin Set) · bodyRadius + centroidY')
