#pragma once
#include "sensors/SyntheticSensors.hpp"
#include "sixdof/SixDof.hpp"

namespace sensors {
// Uses the actual SixDof.cpp body->inertial convention (the legacy header comment is reversed).
// Acceleration is d(R*v_body)/dt = R*(v_dot_body + omega x v_body).
inline GroundTruth fromSixDof(double time, const sixdof::State6& s,
                             const sixdof::StateDerivative6& d, const Vec3& gravity,
                             double pressure_pa, double temperature_k,
                             FlightPhase phase = FlightPhase::PRELAUNCH) {
    GroundTruth t;
    t.time=time; t.position=Vec3(s.X,s.Y,s.Z);
    t.body_to_inertial=Eigen::Quaterniond(s.q0,s.q1,s.q2,s.q3).normalized();
    t.angular_rate_body=Vec3(s.p,s.q,s.r);
    const Vec3 v(s.u,s.v,s.w);
    t.velocity=t.body_to_inertial*v;
    t.acceleration=t.body_to_inertial*(Vec3(d.uDot,d.vDot,d.wDot)+t.angular_rate_body.cross(v));
    t.gravity=gravity; t.pressure_pa=pressure_pa; t.temperature_k=temperature_k; t.phase=phase;
    return t;
}
}
