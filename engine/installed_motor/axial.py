from ..contracts import Node,INERTIA
NODE=Node('axial','Üretim motor modeli eksenel ataleti',INERTIA,{},lambda v,k:k.motor_axial(),implementation='native/motor/axial.cpp',reference='RoketSiM roketsim_cli.cpp::evaluateMassProperties · unsupported roll inertia = 0')
