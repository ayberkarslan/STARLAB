#include "../export.hpp"
#include "constants.hpp"
using namespace starlab_environment;
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_gravity(double altitude){
 return g0*std::pow(r0/(r0+altitude),2.0);
}
