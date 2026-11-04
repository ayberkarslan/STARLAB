#include "threedof/ThreeDof.hpp"
#include "env/Atmosphere.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace threedof {

StateDerivative eom(double t, const State& state, const SimParams& params) {
    StateDerivative deriv;

    double mass = params.massAtTime ? params.massAtTime(t, state) : params.mass;
    double iyy = params.iyyAtTime ? params.iyyAtTime(t, state) : params.iyy;
    double cg = params.cgAtTime ? params.cgAtTime(t, state) : params.cg;
    double thrust = params.thrust ? params.thrust(t) : 0.0;

    double r = state.r;
    double phi = state.phi;
    double vr = state.vr;
    double vphi = state.vphi;
    double theta_in = state.theta_in;
    double omega = state.omega;

    // Local Pitch (angle between local horizontal East and body axis)
    double theta_local = theta_in - phi;

    // Gravity (Inverse square)
    double g = params.g0 * std::pow(params.earthRadius / r, 2);

    // Relative velocity (Atmosphere rotates with Earth)
    double v_rel_r = vr;
    double v_rel_phi = vphi - r * params.earthOmega;

    double V_rel = std::sqrt(v_rel_r * v_rel_r + v_rel_phi * v_rel_phi);
    double gamma_rel = std::atan2(v_rel_r, v_rel_phi);

    double alpha = theta_local - gamma_rel;
    // Normalize alpha to [-pi, pi]
    while (alpha > M_PI) alpha -= 2.0 * M_PI;
    while (alpha < -M_PI) alpha += 2.0 * M_PI;

    double mach = 0.0;
    double q = 0.0;

    AeroCoeffs aero;
    if (params.aeroCallback) {
        env::AtmosphericProperties atmos = env::Atmosphere::getProperties(r - params.earthRadius);
        mach = V_rel / atmos.speedOfSound;
        q = 0.5 * atmos.density * V_rel * V_rel;
        aero = params.aeroCallback(t, mach, alpha, state);
    } else {
        aero.cd = 0.0;
        aero.cl = 0.0;
        aero.cna = 0.0;
        aero.cm_cg = 0.0;
        aero.cmq = 0.0;
    }

    // Aerodynamic Forces
    double Drag = q * params.sref * aero.cd;
    double Lift = q * params.sref * aero.cl;
    
    double NormalForce = 0.0;
    if (aero.cl != 0.0 || aero.cd != 0.0) {
        NormalForce = Lift * std::cos(alpha) + Drag * std::sin(alpha);
    } else if (aero.cna != 0.0) {
        NormalForce = q * params.sref * aero.cna * alpha;
        Lift = NormalForce * std::cos(alpha) - Drag * std::sin(alpha);
    }

    double D_phi = -Drag * std::cos(gamma_rel);
    double D_r   = -Drag * std::sin(gamma_rel);

    double L_phi = -Lift * std::sin(gamma_rel);
    double L_r   =  Lift * std::cos(gamma_rel);

    double F_aero_phi = D_phi + L_phi;
    double F_aero_r   = D_r + L_r;

    // Thrust
    double T_r = thrust * std::sin(theta_local);
    double T_phi = thrust * std::cos(theta_local);

    // Equations of Motion (Inertial Equatorial)
    deriv.rDot = vr;
    deriv.phiDot = vphi / r;
    
    deriv.vrDot = (vphi * vphi / r) - g + (F_aero_r + T_r) / mass;
    deriv.vphiDot = -(vr * vphi / r) + (F_aero_phi + T_phi) / mass;
    
    deriv.theta_inDot = omega;

    // Pitch Moment
    double pitch_damping_moment = 0.0;
    if (V_rel > 1.0 && params.dref > 0.0) {
        double q_rel = omega - deriv.phiDot;
        pitch_damping_moment = aero.cmq * q * params.sref * params.dref * (q_rel * params.dref / (2.0 * V_rel));
    }
    
    double M_y = aero.cm_cg * q * params.sref * params.dref + pitch_damping_moment;
    
    deriv.omegaDot = M_y / iyy;

    return deriv;
}

State addScaled(const State& state, const StateDerivative& derivative, double scale) {
    State next;
    next.r = state.r + derivative.rDot * scale;
    next.phi = state.phi + derivative.phiDot * scale;
    next.vr = state.vr + derivative.vrDot * scale;
    next.vphi = state.vphi + derivative.vphiDot * scale;
    next.theta_in = state.theta_in + derivative.theta_inDot * scale;
    next.omega = state.omega + derivative.omegaDot * scale;
    return next;
}

State addWeighted(
    const State& state,
    const StateDerivative& k1,
    const StateDerivative& k2,
    const StateDerivative& k3,
    const StateDerivative& k4,
    double dt
) {
    State next;
    next.r = state.r + (dt / 6.0) * (k1.rDot + 2.0 * k2.rDot + 2.0 * k3.rDot + k4.rDot);
    next.phi = state.phi + (dt / 6.0) * (k1.phiDot + 2.0 * k2.phiDot + 2.0 * k3.phiDot + k4.phiDot);
    next.vr = state.vr + (dt / 6.0) * (k1.vrDot + 2.0 * k2.vrDot + 2.0 * k3.vrDot + k4.vrDot);
    next.vphi = state.vphi + (dt / 6.0) * (k1.vphiDot + 2.0 * k2.vphiDot + 2.0 * k3.vphiDot + k4.vphiDot);
    next.theta_in = state.theta_in + (dt / 6.0) * (k1.theta_inDot + 2.0 * k2.theta_inDot + 2.0 * k3.theta_inDot + k4.theta_inDot);
    next.omega = state.omega + (dt / 6.0) * (k1.omegaDot + 2.0 * k2.omegaDot + 2.0 * k3.omegaDot + k4.omegaDot);
    return next;
}

} // namespace threedof
