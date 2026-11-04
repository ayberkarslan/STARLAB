#include "env/Atmosphere.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace env {

// Constants for US Standard Atmosphere 1976
static constexpr double R_star = 8314.32; // Universal gas constant [J/(kmol K)]
static constexpr double M0 = 28.9644; // Sea-level mean molecular weight [kg/kmol]
static constexpr double R_spec = R_star / M0; // Specific gas constant [J/(kg K)] ~ 287.05287
static constexpr double gamma = 1.40; // Ratio of specific heats
static constexpr double r0 = 6356766.0; // Effective Earth radius for geopotential [m]

static double sutherlandViscosity(double temperatureK) {
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

AtmosphericProperties Atmosphere::getProperties(double altitude) {
    AtmosphericProperties props;
    props.altitude = altitude;

    // Above 1000 km, assume zero density
    if (altitude > 1000000.0) {
        props.temperature = 2.7; // Cosmic background
        props.pressure = 0.0;
        props.density = 0.0;
        props.speedOfSound = std::sqrt(gamma * R_spec * props.temperature);
        props.dynamicViscosity = sutherlandViscosity(props.temperature);
        return props;
    }

    // Convert geometric altitude to geopotential altitude
    double H = (r0 * altitude) / (r0 + altitude);

    int b = 0;
    for (int i = 0; i < NUM_LAYERS; ++i) {
        if (H >= layers[i].Hb) {
            b = i;
        } else {
            break;
        }
    }

    const AtmosLayer& layer = layers[b];
    
    double T = layer.Tb + layer.Lb * (H - layer.Hb);
    if (T < 1.0) T = 1.0; // Prevent unphysical temperatures

    double P;
    if (layer.Lb == 0.0) {
        P = layer.Pb * std::exp((-g0 * (H - layer.Hb)) / (R_spec * layer.Tb));
    } else {
        P = layer.Pb * std::pow(layer.Tb / T, (g0) / (R_spec * layer.Lb));
    }

    if (P < 0.0) P = 0.0;

    double rho = P / (R_spec * T);
    
    // Smooth out zero density above the 7th layer if we go extremely high
    if (altitude > 86000.0) {
        // Just let it extrapolate normally or decay rapidly, the 7th layer has negative lapse rate
        // so temperature drops until 0, which would crash. We clamped T >= 1.0.
        // For accurate 86km-1000km, we need more layers (thermosphere), but this is usually enough for drag.
    }

    double a = std::sqrt(gamma * R_spec * T);

    props.temperature = T;
    props.pressure = P;
    props.density = rho;
    props.speedOfSound = a;
    props.dynamicViscosity = sutherlandViscosity(T);

    return props;
}

AtmosphericProperties Atmosphere::getPropertiesFromSurface(
    double altitudeAgl, double surfaceTemperatureK, double surfacePressurePa,
    double lapseRateKPerM) {
    if (!std::isfinite(surfaceTemperatureK) || surfaceTemperatureK <= 0.0) {
        throw std::invalid_argument("surface temperature must be positive Kelvin");
    }
    if (!std::isfinite(surfacePressurePa) || surfacePressurePa <= 0.0) {
        throw std::invalid_argument("surface pressure must be positive Pascal");
    }
    if (!std::isfinite(lapseRateKPerM) || lapseRateKPerM <= 0.0) {
        throw std::invalid_argument("lapse rate must be positive");
    }

    constexpr double troposphereTopM = 11000.0;
    const double h = std::max(altitudeAgl, 0.0);
    const double layerHeight = std::min(h, troposphereTopM);
    double temperature = std::max(
        surfaceTemperatureK - lapseRateKPerM * layerHeight, 1.0);
    double pressure = surfacePressurePa * std::pow(
        temperature / surfaceTemperatureK,
        g0 / (R_spec * lapseRateKPerM));
    if (h > troposphereTopM) {
        const double tropopauseTemperature = temperature;
        const double pressureAtTropopause = pressure;
        pressure = pressureAtTropopause * std::exp(
            -g0 * (h - troposphereTopM) / (R_spec * tropopauseTemperature));
    }

    AtmosphericProperties props;
    props.altitude = h;
    props.temperature = temperature;
    props.pressure = pressure;
    props.density = pressure / (R_spec * temperature);
    props.speedOfSound = std::sqrt(gamma * R_spec * temperature);
    props.dynamicViscosity = sutherlandViscosity(temperature);
    return props;
}

} // namespace env
