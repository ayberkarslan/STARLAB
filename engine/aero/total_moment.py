from ..contracts import Node,Binding
from . import MOMENT
NODE=Node('totalMoment','Toplam yunuslama momenti',MOMENT,{'aero':Binding('pitchMoment',MOMENT),'damping':Binding('dampingMoment',MOMENT)},lambda v,k:k.aero_total_moment(v['aero'],v['damping']),implementation='native/aero/total_moment.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
