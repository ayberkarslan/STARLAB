#include "../export.hpp"
#include "constants.hpp"
using namespace starlab_environment;
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_speed_of_sound(double temperature){
 return std::sqrt(starlab_environment::gamma*R_spec*temperature);
}
