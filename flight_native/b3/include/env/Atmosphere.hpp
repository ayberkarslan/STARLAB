#pragma once

namespace env {

struct AtmosphericProperties {
    double altitude;     // Altitude above mean sea level [m]
    double temperature;  // Absolute temperature [K]
    double pressure;     // Pressure [Pa]
    double density;      // Density [kg/m^3]
    double speedOfSound; // Speed of sound [m/s]
    double dynamicViscosity; // Dynamic viscosity [Pa s]
};

class Atmosphere {
public:
    // Computes atmospheric properties using the U.S. Standard Atmosphere 1976 model.
    // Valid from -600 m to 86,000 m (geometric altitude).
    // Above 86km, it will extrapolate the last layer or clamp, but accurately it's undefined.
    // For our purposes, above 86km density is negligible and we can cap it or compute 
    // an approximate exponential decay. NASA ACC17 requires US 1976.
    static AtmosphericProperties getProperties(double altitude);

    // Dry-air atmosphere anchored to measurements at the launch point.
    // altitudeAgl is height above the launch point, never ASL altitude.
    static AtmosphericProperties getPropertiesFromSurface(
        double altitudeAgl,
        double surfaceTemperatureK,
        double surfacePressurePa,
        double lapseRateKPerM = 0.0065);

    // Standard gravity at sea level [m/s^2]
    static constexpr double g0 = 9.80665;
    
    // Earth's mean radius [m]
    static constexpr double earthRadius = 6356766.0; // WGS-84 polar radius or mean radius
    // Wait, let's use the nominal earth radius used in US 1976
    static constexpr double earthRadius1976 = 6369000.0;
};

} // namespace env
