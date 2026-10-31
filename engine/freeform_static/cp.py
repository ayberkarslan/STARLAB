from ..contracts import Node,Binding
from ..static_aero import CP
from . import PORTS

NODE=Node('cp','Serbest kanat yerel CP',CP,{k:Binding(k,PORTS[k]) for k in ('points','mach','cpOverrideEnabled','cpOverride')},
 lambda v,k:k.freeform_static_cp(v['points'],len(v['points'])//2,v['mach'],v['cpOverrideEnabled'],v['cpOverride']),
 implementation='native/freeform/cp.cpp',reference='RoketSiM src/components/FreeformFinSet.cpp::rawAeroProperties + include/aero/FinPlanform.hpp')
