#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_absolute_altitude(double launchAltitude,double height){
 return launchAltitude+std::max(height,0.0);
}
