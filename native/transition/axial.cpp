#include <algorithm>
#include <cmath>
#include "../export.hpp"
#include "properties.hpp"
STARLAB_EXPORT double transition_axial(double length,double fore,double aft,double wall,double density){double sum=0,dz=length/2000.0;for(int i=0;i<2000;++i){const double z=(i+.5)*dz,ro=fore+(aft-fore)*z/length,ri=std::max(0.0,fore-wall)+(std::max(0.0,aft-wall)-std::max(0.0,fore-wall))*z/length;sum+=.5*density*StarlabTransition::pi*(std::pow(ro,4)-std::pow(ri,4))*dz;}return sum;}
