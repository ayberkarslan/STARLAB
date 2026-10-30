from ..contracts import Node,Binding
from . import AREA,AREA_MOMENT,POSITION
NODE=Node('bodyCp','Gövde CP',POSITION,{'bodyArea':Binding('bodyArea',AREA),'bodyMoment':Binding('bodyMoment',AREA_MOMENT),'cp':Binding('cp',POSITION)},lambda v,k:k.aero_body_cp(v['bodyArea'],v['bodyMoment'],v['cp']),implementation='native/aero/body_cp.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
