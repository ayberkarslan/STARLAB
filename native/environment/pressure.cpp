#include "../export.hpp"
#include "constants.hpp"
using namespace starlab_environment;
// RoketSiM Atmosphere/WindModel/CLI production scalar calculation.
STARLAB_EXPORT double environment_pressure(double model,double height,double altitude,double surfaceTemperature,double surfacePressure,double temperature){
 if(model==0.0){const double h=std::max(height,0.0);double pressure=surfacePressure*std::pow(temperature/surfaceTemperature,g0/(R_spec*0.0065));if(h>11000.0)pressure=pressure*std::exp(-g0*(h-11000.0)/(R_spec*temperature));return pressure;}const auto& layer=layerAt(altitude);const double H=(r0*altitude)/(r0+altitude);double P;if(layer.Lb==0.0)P=layer.Pb*std::exp((-g0*(H-layer.Hb))/(R_spec*layer.Tb));else P=layer.Pb*std::pow(layer.Tb/temperature,g0/(R_spec*layer.Lb));if(P<0.0)P=0.0;return P;
}
