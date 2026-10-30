#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_wind_direction(double direction,double gradient,double height){
 double normalized=std::fmod(direction+gradient*std::max(height,0.0)/1000.0,360.0);if(normalized<0.0)normalized+=360.0;return normalized;
}
