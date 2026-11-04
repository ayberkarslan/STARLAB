#pragma once

#include <functional>
#include <limits>
#include <Eigen/Dense>

namespace threedof {

// 3-DOF State for Equatorial 2D trajectory (Spherical rotating earth)
struct State {
    double r = 6356766.0;      // Distance from Earth center [m]
    double phi = 0.0;          // Longitude (downrange angle) [rad]
    double vr = 0.0;           // Radial (Upward) inertial velocity [m/s]
    double vphi = 0.0;         // Tangential (Eastward) inertial velocity [m/s]
    double theta_in = 0.0;     // Inertial body pitch angle [rad]
    double omega = 0.0;        // Body pitch rate [rad/s]
};

struct StateDerivative {
    double rDot = 0.0;
    double phiDot = 0.0;
    double vrDot = 0.0;
    double vphiDot = 0.0;
    double theta_inDot = 0.0;
    double omegaDot = 0.0;
};

// Dynamic Aerodynamics Coefficients returned by callback
struct AeroCoeffs {
    double cd = 0.0;
    double cl = 0.0;    // Lift coefficient (optional, if using full aero)
    double cna = 0.0;   // Normal force derivative
    double cm_cg = 0.0; // Pitching moment coefficient about CG
    double cmq = 0.0;   // Pitch damping derivative
};

struct SimParams {
    double mass = 1.0;
    double iyy = 1.0;
    double cg = 0.0;      // Center of gravity from nose tip [m]
    double sref = 0.0;
    double dref = 0.0;    // Reference length for damping

    // Environment
    double g0 = 9.80665;
    double earthRadius = 6356766.0; // Nominal earth radius
    double earthOmega = 7.292115e-5; // Earth rotation rate [rad/s]

    // Time when stage is dropped or properties change discretely
    double variablePropertiesEndTime = std::numeric_limits<double>::infinity();

    // Callbacks for dynamic properties
    std::function<double(double, const State&)> massAtTime;
    std::function<double(double, const State&)> iyyAtTime;
    std::function<double(double, const State&)> cgAtTime;
    std::function<double(double)> thrust;
    
    // Dynamic aero callback
    std::function<AeroCoeffs(double t, double mach, double alpha, const State&)> aeroCallback;
};

StateDerivative eom(double t, const State& state, const SimParams& params);
State addScaled(const State& state, const StateDerivative& derivative, double scale);
State addWeighted(
    const State& state,
    const StateDerivative& k1,
    const StateDerivative& k2,
    const StateDerivative& k3,
    const StateDerivative& k4,
    double dt
);

} // namespace threedof
