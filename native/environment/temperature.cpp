#include "../export.hpp"
#include "constants.hpp"
using namespace starlab_environment;
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_temperature(double model,double height,double altitude,double surfaceTemperature){
 if(model==0.0){const double h=std::max(height,0.0);const double layerHeight=std::min(h,11000.0);return std::max(surfaceTemperature-0.0065*layerHeight,1.0);}const auto& layer=layerAt(altitude);const double H=(r0*altitude)/(r0+altitude);return std::max(layer.Tb+layer.Lb*(H-layer.Hb),1.0);
}
