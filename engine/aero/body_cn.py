from ..contracts import Node,Binding
from . import ANGLE,AREA,COEFFICIENT,MACH,REF_AREA
NODE=Node('bodyCn','Gövde normal katsayısı',COEFFICIENT,{'mach':Binding('mach',MACH),'alpha':Binding('flowAlpha',ANGLE),'bodyArea':Binding('bodyArea',AREA),'area':Binding('area',REF_AREA)},lambda v,k:k.aero_body_cn(v['mach'],v['alpha'],v['bodyArea'],v['area']),implementation='native/aero/body_cn.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
