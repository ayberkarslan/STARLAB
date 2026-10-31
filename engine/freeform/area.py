from ..contracts import Node,Binding
from . import POINTS,AREA

NODE=Node('area','Kanat planform alanı',AREA,{'points':Binding('points',POINTS)},
 lambda v,k:k.freeform_planform_area(v['points'],len(v['points'])//2),
 implementation='native/freeform/area.cpp',reference='RoketSiM include/aero/FinPlanform.hpp::calculate::area')
