from ..contracts import Node,Binding,LENGTH
from . import DISTANCE,MARGIN
NODE=Node('stabilityMargin','Statik marj',MARGIN,{'lever':Binding('lever',DISTANCE),'diameter':Binding('diameter',LENGTH)},lambda v,k:k.aero_stability_margin(v['lever'],v['diameter']),implementation='native/aero/stability_margin.cpp',reference='RoketSiM src/cli/design_analysis.hpp::stability_margin_cal')
