#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_dynamic_pressure(double density,double speed){
 return 0.5*density*speed*speed;
}
