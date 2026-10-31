from ..contracts import Node,Binding,VOLUME,DENSITY,MASS
NODE=Node('mass','Kütle',MASS,{'volume':Binding('volume',VOLUME),'density':Binding('density',DENSITY)},lambda v,k:k.transition_mass(v['volume'],v['density']),implementation='native/transition/mass.cpp',reference='RoketSiM src/components/Transition.cpp::rawMassProperties · density × volume')
