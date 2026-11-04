#pragma once

#include <functional>
#include "env/Geodesy.hpp"

namespace sixdof {

struct State6 {
    // Position in Inertial Frame
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;

    // Velocity in Body Frame
    double u = 0.0;
    double v = 0.0;
    double w = 0.0;

    // Attitude Quaternions (Inertial to Body)
    double q0 = 1.0; // Scalar part
    double q1 = 0.0;
    double q2 = 0.0;
    double q3 = 0.0;

    // Angular Rates in Body Frame
    double p = 0.0;
    double q = 0.0;
    double r = 0.0;
};

struct StateDerivative6 {
    double XDot, YDot, ZDot;
    double uDot, vDot, wDot;
    double q0Dot, q1Dot, q2Dot, q3Dot;
    double pDot, qDot_rate, rDot;
};

struct ForceMoment {
    double Fx = 0.0, Fy = 0.0, Fz = 0.0; // Body frame forces
    double Mx = 0.0, My = 0.0, Mz = 0.0; // Body frame moments
};

struct MassProps {
    double mass = 1.0;
    
    // Inertia Tensor
    double Ixx = 1.0;
    double Iyy = 1.0;
    double Izz = 1.0;

    // Cross products of inertia
    double Ixy = 0.0;
    double Ixz = 0.0;
    double Iyz = 0.0;
};

struct SimParams6 {
    // Dynamic Callbacks
    std::function<MassProps(double t, const State6& s)> massCallback;
    std::function<ForceMoment(double t, const State6& s)> forceMomentCallback;
};

// Pure rigid body dynamics equation of motion
StateDerivative6 eom6(double t, const State6& state, const SimParams6& params);

// Integrator helpers
State6 addScaled(const State6& state, const StateDerivative6& derivative, double scale);
State6 addWeighted(const State6& state, 
                   const StateDerivative6& k1, 
                   const StateDerivative6& k2, 
                   const StateDerivative6& k3, 
                   const StateDerivative6& k4, 
                   double dt);

// Utility to normalize quaternions
void normalizeQuaternion(State6& state);

// Adapter for State6 X/Y/Z interpreted as local
// downrange/crossrange-right/up coordinates.
geodesy::GeodeticPosition geographicPosition(
    const State6& state,
    const geodesy::GeodeticPosition& launchPosition,
    double launchAzimuthDeg);

} // namespace sixdof
