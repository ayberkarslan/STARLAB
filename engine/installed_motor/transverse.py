from ..contracts import Node,Binding,MASS,INERTIA
from .inertia_coefficient import COEFFICIENT
NODE=Node('transverse','Motor enine ataleti',INERTIA,{'dry':Binding('dry',MASS),'remaining':Binding('remaining',MASS),'coefficient':Binding('inertiaCoefficient',COEFFICIENT)},lambda v,k:k.motor_transverse(v['dry'],v['remaining'],v['coefficient']),implementation='native/motor/transverse.cpp',reference='RoketSiM roketsim_cli.cpp::evaluateMassProperties · dry/propellant contributions')
