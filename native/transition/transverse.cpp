#include <algorithm>
#include <cmath>
#include "../export.hpp"
#include "properties.hpp"
STARLAB_EXPORT double transition_transverse(double length,double fore,double aft,double wall,double density,double cg){double sum=0,dz=length/2000.0,fi=std::max(0.0,fore-wall),ai=std::max(0.0,aft-wall);for(int i=0;i<2000;++i){const double z=(i+.5)*dz,ro=fore+(aft-fore)*z/length,ri=fi+(ai-fi)*z/length,dmdz=density*StarlabTransition::pi*(ro*ro-ri*ri);sum+=dmdz*(.25*(ro*ro+ri*ri)+(z-cg)*(z-cg))*dz;}return sum;}
