from ..contracts import Node,Binding
from ..static_aero import CNA
from . import PORTS

NODE=Node('cna','Serbest kanat CNa',CNA,{k:Binding(k,PORTS[k]) for k in ('points','count','bodyRadius','referenceArea','mach','alpha','cnaOverrideEnabled','cnaOverride')},
 lambda v,k:k.freeform_static_cna(v['points'],len(v['points'])//2,v['count'],v['bodyRadius'],v['referenceArea'],v['mach'],v['alpha'],v['cnaOverrideEnabled'],v['cnaOverride']),
 implementation='native/freeform/cna.cpp',reference='RoketSiM src/components/FreeformFinSet.cpp::rawAeroProperties + include/aero/FinPlanform.hpp')
