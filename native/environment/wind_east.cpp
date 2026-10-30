#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_wind_east(double speed,double direction){
 return -speed*std::sin(direction*3.14159265358979323846/180.0);
}
