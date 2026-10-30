from ..contracts import Node,Binding,MASS
NODE=Node('mass','Anlık toplam kütle',MASS,{'dry':Binding('dry',MASS),'remaining':Binding('remaining',MASS)},lambda v,k:k.motor_mass(v['dry'],v['remaining']),implementation='native/motor/mass.cpp',reference='RoketSiM src/propulsion/MotorModel.cpp::mass · dry + propellant')
