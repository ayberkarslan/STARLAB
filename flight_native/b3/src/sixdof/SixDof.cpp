#include "sixdof/SixDof.hpp"
#include <cmath>
#include <iostream>

namespace sixdof {

StateDerivative6 eom6(double t, const State6& state, const SimParams6& params) {
    StateDerivative6 deriv;

    // Callbacks for dynamic properties
    MassProps mp = params.massCallback ? params.massCallback(t, state) : MassProps();
    ForceMoment fm = params.forceMomentCallback ? params.forceMomentCallback(t, state) : ForceMoment();

    // 1. Attitude Kinematics (Quaternions)
    // q represents rotation FROM Body TO Inertial
    double q0 = state.q0;
    double q1 = state.q1;
    double q2 = state.q2;
    double q3 = state.q3;

    double p = state.p;
    double q = state.q;
    double r = state.r;

    deriv.q0Dot = 0.5 * (-p*q1 - q*q2 - r*q3);
    deriv.q1Dot = 0.5 * ( p*q0 - q*q3 + r*q2);
    deriv.q2Dot = 0.5 * ( q*q0 + p*q3 - r*q1);
    deriv.q3Dot = 0.5 * ( r*q0 - p*q2 + q*q1);

    // 2. Translational Kinematics (Velocity from Body to Inertial)
    // Direction Cosine Matrix (Body to Inertial)
    double R11 = q0*q0 + q1*q1 - q2*q2 - q3*q3;
    double R12 = 2.0 * (q1*q2 - q0*q3);
    double R13 = 2.0 * (q1*q3 + q0*q2);

    double R21 = 2.0 * (q1*q2 + q0*q3);
    double R22 = q0*q0 - q1*q1 + q2*q2 - q3*q3;
    double R23 = 2.0 * (q2*q3 - q0*q1);

    double R31 = 2.0 * (q1*q3 - q0*q2);
    double R32 = 2.0 * (q2*q3 + q0*q1);
    double R33 = q0*q0 - q1*q1 - q2*q2 + q3*q3;

    double u = state.u;
    double v = state.v;
    double w = state.w;

    deriv.XDot = R11*u + R12*v + R13*w;
    deriv.YDot = R21*u + R22*v + R23*w;
    deriv.ZDot = R31*u + R32*v + R33*w;

    // 3. Translational Dynamics (Newton's Second Law in rotating Body frame)
    double mass = mp.mass;
    if (mass <= 0.0) mass = 1e-6; // Prevent division by zero

    deriv.uDot = (fm.Fx / mass) + r*v - q*w;
    deriv.vDot = (fm.Fy / mass) - r*u + p*w;
    deriv.wDot = (fm.Fz / mass) + q*u - p*v;

    // 4. Rotational Dynamics (Euler's Equations)
    // Assuming principal axes of inertia (Ixy = Ixz = Iyz = 0) for simplicity.
    // d(Iw)/dt + w x (Iw) = M. I-dot captures continuous propellant
    // depletion rather than treating inertia as constant within Euler's law.
    double Ixx = mp.Ixx > 0.0 ? mp.Ixx : 1e-6;
    double Iyy = mp.Iyy > 0.0 ? mp.Iyy : 1e-6;
    double Izz = mp.Izz > 0.0 ? mp.Izz : 1e-6;

    // Propellant leaving the vehicle also carries angular momentum. The
    // corresponding exhaust-flux term is not represented by MassProps, so an
    // isolated -I_dot * omega term would be unphysical here.
    deriv.pDot = (fm.Mx + (Iyy - Izz) * q * r) / Ixx;
    deriv.qDot_rate = (fm.My + (Izz - Ixx) * p * r) / Iyy;
    deriv.rDot = (fm.Mz + (Ixx - Iyy) * p * q) / Izz;

    return deriv;
}

State6 addScaled(const State6& state, const StateDerivative6& derivative, double scale) {
    State6 next = state;
    next.X += derivative.XDot * scale;
    next.Y += derivative.YDot * scale;
    next.Z += derivative.ZDot * scale;

    next.u += derivative.uDot * scale;
    next.v += derivative.vDot * scale;
    next.w += derivative.wDot * scale;

    next.q0 += derivative.q0Dot * scale;
    next.q1 += derivative.q1Dot * scale;
    next.q2 += derivative.q2Dot * scale;
    next.q3 += derivative.q3Dot * scale;

    next.p += derivative.pDot * scale;
    next.q += derivative.qDot_rate * scale;
    next.r += derivative.rDot * scale;
    return next;
}

State6 addWeighted(const State6& state, 
                   const StateDerivative6& k1, 
                   const StateDerivative6& k2, 
                   const StateDerivative6& k3, 
                   const StateDerivative6& k4, 
                   double dt) {
    State6 next = state;
    next.X += (dt / 6.0) * (k1.XDot + 2.0*k2.XDot + 2.0*k3.XDot + k4.XDot);
    next.Y += (dt / 6.0) * (k1.YDot + 2.0*k2.YDot + 2.0*k3.YDot + k4.YDot);
    next.Z += (dt / 6.0) * (k1.ZDot + 2.0*k2.ZDot + 2.0*k3.ZDot + k4.ZDot);

    next.u += (dt / 6.0) * (k1.uDot + 2.0*k2.uDot + 2.0*k3.uDot + k4.uDot);
    next.v += (dt / 6.0) * (k1.vDot + 2.0*k2.vDot + 2.0*k3.vDot + k4.vDot);
    next.w += (dt / 6.0) * (k1.wDot + 2.0*k2.wDot + 2.0*k3.wDot + k4.wDot);

    next.q0 += (dt / 6.0) * (k1.q0Dot + 2.0*k2.q0Dot + 2.0*k3.q0Dot + k4.q0Dot);
    next.q1 += (dt / 6.0) * (k1.q1Dot + 2.0*k2.q1Dot + 2.0*k3.q1Dot + k4.q1Dot);
    next.q2 += (dt / 6.0) * (k1.q2Dot + 2.0*k2.q2Dot + 2.0*k3.q2Dot + k4.q2Dot);
    next.q3 += (dt / 6.0) * (k1.q3Dot + 2.0*k2.q3Dot + 2.0*k3.q3Dot + k4.q3Dot);

    next.p += (dt / 6.0) * (k1.pDot + 2.0*k2.pDot + 2.0*k3.pDot + k4.pDot);
    next.q += (dt / 6.0) * (k1.qDot_rate + 2.0*k2.qDot_rate + 2.0*k3.qDot_rate + k4.qDot_rate);
    next.r += (dt / 6.0) * (k1.rDot + 2.0*k2.rDot + 2.0*k3.rDot + k4.rDot);
    
    normalizeQuaternion(next);
    return next;
}

void normalizeQuaternion(State6& state) {
    double norm = std::sqrt(state.q0*state.q0 + state.q1*state.q1 + 
                            state.q2*state.q2 + state.q3*state.q3);
    if (norm > 1e-8) {
        state.q0 /= norm;
        state.q1 /= norm;
        state.q2 /= norm;
        state.q3 /= norm;
    } else {
        state.q0 = 1.0;
        state.q1 = 0.0;
        state.q2 = 0.0;
        state.q3 = 0.0;
    }
}

geodesy::GeodeticPosition geographicPosition(
    const State6& state,
    const geodesy::GeodeticPosition& launchPosition,
    double launchAzimuthDeg) {
    const auto enu = geodesy::Wgs84::launchFrameToEnu(
        state.X, state.Y, state.Z, launchAzimuthDeg);
    return geodesy::Wgs84::enuToGeodetic(enu, launchPosition);
}

} // namespace sixdof
