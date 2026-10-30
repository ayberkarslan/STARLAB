#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_reynolds(double speed,double length,double viscosity,double density){
 if(speed<=0.1)return 0.0;
 const double kinematicViscosity=density>0.0?viscosity/density:1.0e30;return speed*length/std::max(kinematicViscosity,1e-30);
}
