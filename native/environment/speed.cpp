#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_speed(double horizontal,double vertical){
 return std::sqrt(vertical*vertical+horizontal*horizontal);
}
