#pragma once

struct FlightCondition {
    double mach = 0.0;
    double reynolds = 0.0;
    double alpha = 0.0;              // rad
    double velocity = 0.0;           // m/s
    double rho = 1.225;              // kg/m^3
    double qbar = 0.0;               // Pa
    double referenceArea = 0.0;      // m^2
    double referenceDiameter = 0.0;  // m
    double referenceLength = 0.0;    // m, roll/pitch moment reference
    double rollRate = 0.0;           // rad/s
    bool motorOn = false;
};
