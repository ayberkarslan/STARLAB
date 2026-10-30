#pragma once
#include <cmath>
#include <algorithm>
namespace starlab_environment {
// Constants for US Standard Atmosphere 1976
static constexpr double R_star = 8314.32; // Universal gas constant [J/(kmol K)]
static constexpr double M0 = 28.9644; // Sea-level mean molecular weight [kg/kmol]
static constexpr double R_spec = R_star / M0; // Specific gas constant [J/(kg K)] ~ 287.05287
static constexpr double gamma = 1.40; // Ratio of specific heats
static constexpr double r0 = 6356766.0; // Effective Earth radius for geopotential [m]

inline double sutherlandViscosity(double temperatureK) {
    constexpr double referenceTemperatureK = 273.15;
    constexpr double referenceViscosity = 1.716e-5;
    constexpr double sutherlandK = 110.4;
    return referenceViscosity * std::pow(temperatureK / referenceTemperatureK, 1.5)
        * (referenceTemperatureK + sutherlandK) / (temperatureK + sutherlandK);
}

struct AtmosLayer {
    double Hb; // Base geopotential altitude [m]
    double Tb; // Base temperature [K]
    double Pb; // Base pressure [Pa]
    double Lb; // Temperature lapse rate [K/m]
};

// US 1976 atmosphere layers (up to 86 km geometric / 84.852 km geopotential)
static constexpr int NUM_LAYERS = 7;
static constexpr AtmosLayer layers[NUM_LAYERS] = {
    {0.0, 288.15, 101325.0, -0.0065},
    {11000.0, 216.65, 22632.10, 0.0},
    {20000.0, 216.65, 5474.89, 0.001},
    {32000.0, 228.65, 868.019, 0.0028},
    {47000.0, 270.65, 110.906, 0.0},
    {51000.0, 270.65, 66.9389, -0.0028},
    {71000.0, 214.65, 3.95642, -0.002}
};

constexpr double g0=9.80665;
inline const AtmosLayer& layerAt(double altitude){
 const double H=(r0*altitude)/(r0+altitude);
 int b=0;for(int i=0;i<NUM_LAYERS;++i){if(H>=layers[i].Hb)b=i;else break;}
 return layers[b];
}
}
