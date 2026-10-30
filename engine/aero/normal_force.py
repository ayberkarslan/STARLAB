from ..contracts import Node,Binding
from . import COEFFICIENT,NORMAL_FORCE,PRESSURE,REF_AREA
NODE=Node('normalForce','Normal aero kuvveti',NORMAL_FORCE,{'coefficient':Binding('normalCoefficient',COEFFICIENT),'pressure':Binding('dynamicPressure',PRESSURE),'area':Binding('area',REF_AREA)},lambda v,k:k.aero_normal_force(v['coefficient'],v['pressure'],v['area']),implementation='native/aero/normal_force.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
