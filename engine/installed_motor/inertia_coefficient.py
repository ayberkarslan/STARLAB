from ..contracts import Node,Binding,LENGTH,Port
COEFFICIENT=Port('transverse-inertia-per-mass','m²')
WET=Port('positive-motor-mass','kg',positive=True)
NODE=Node('inertiaCoefficient','Motor atalet katsayısı',COEFFICIENT,{'wet':Binding('wet',WET),'radius':Binding('radius',LENGTH),'length':Binding('length',LENGTH)},lambda v,k:k.motor_inertia_coefficient(v['wet'],v['radius'],v['length']),implementation='native/motor/inertia_coefficient.cpp',reference='RoketSiM design_analysis.hpp Motor + motor_instances transverse_inertia_per_mass')
