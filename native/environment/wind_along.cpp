#include "../export.hpp"
#include <cmath>
#include <algorithm>
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_wind_along(double east,double north,double heading){
 const double azimuth=heading*3.14159265358979323846/180.0;return east*std::sin(azimuth)+north*std::cos(azimuth);
}
