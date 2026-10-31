from ..contracts import Node,Binding,VOLUME,DENSITY,MASS
NODE=Node('mass','Kütle',MASS,{'volume':Binding('volume',VOLUME),'density':Binding('density',DENSITY)},lambda v,k:k.rail_button_mass(v['volume'],v['density']),implementation='native/rail_button/mass.cpp',reference='RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Rail Button) · density × volume')
