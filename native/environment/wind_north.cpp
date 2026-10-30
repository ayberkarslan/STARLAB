#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_wind_north(double speed,double direction){
 return -speed*std::cos(direction*3.14159265358979323846/180.0);
}
