#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_wind_multiplier(double model,double height,double reference,double minimum,double exponent,double roughness){
 if(model==0.0)return 1.0;double h=std::max(height,0.0);if(model==2.0){if(h<=roughness)return 0.0;return std::max(std::log(h/roughness)/std::log(reference/roughness),0.0);}h=std::max(h,minimum);if(h<=0.0)return 0.0;return std::pow(h/reference,exponent);
}
