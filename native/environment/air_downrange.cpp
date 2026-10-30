#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_air_downrange(double ground,double wind){
 return ground-wind;
}
