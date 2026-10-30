from ..contracts import Node,Binding
from . import ANGLE,COEFFICIENT,SLOPE
NODE=Node('normalCoefficient','Toplam normal katsayı',COEFFICIENT,{'cna':Binding('cna',SLOPE),'alpha':Binding('effectiveAlpha',ANGLE),'bodyCn':Binding('bodyCn',COEFFICIENT)},lambda v,k:k.aero_normal_coefficient(v['cna'],v['alpha'],v['bodyCn']),implementation='native/aero/normal_coefficient.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
