from ..contracts import Node,Binding
from . import ANGLE,AXIAL_FORCE,DRAG,PRESSURE,REF_AREA,SPEED
NODE=Node('axialForce','Eksenel aero kuvveti',AXIAL_FORCE,{'speed':Binding('speed',SPEED),'alpha':Binding('flowAlpha',ANGLE),'pressure':Binding('dynamicPressure',PRESSURE),'area':Binding('area',REF_AREA),'cd':Binding('cd',DRAG)},lambda v,k:k.aero_axial_force(v['speed'],v['alpha'],v['pressure'],v['area'],v['cd']),implementation='native/aero/axial_force.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · supplied coefficients, non-RASAero branch')
