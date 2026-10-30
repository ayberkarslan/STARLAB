#include "../export.hpp"
#include "constants.hpp"
using namespace starlab_environment;
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_density(double pressure,double temperature){
 return pressure/(R_spec*temperature);
}
