#include "aero/Aero2D.hpp"

#include <cmath>

namespace aero2d {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEpsilon = 1e-9;

double cross2(const Eigen::Vector2d& a, const Eigen::Vector2d& b) {
    return a.x() * b.y() - a.y() * b.x();
}

Eigen::Vector2d relativeVelocity(const AeroState& state, const AeroParams& params) {
    return Eigen::Vector2d(state.vx, state.vz) - params.wind;
}

Eigen::Vector2d bodyAxis(double theta) {
    return Eigen::Vector2d(std::cos(theta), std::sin(theta));
}

Eigen::Vector2d normalAxis(double theta) {
    return Eigen::Vector2d(-std::sin(theta), std::cos(theta));
}

double dynamicPressure(double speed, double airDensity) {
    return 0.5 * airDensity * speed * speed;
}

} // namespace

double wrapAngle(double angle) {
    while (angle > kPi) {
        angle -= 2.0 * kPi;
    }
    while (angle < -kPi) {
        angle += 2.0 * kPi;
    }
    return angle;
}

double angleOfAttack(const AeroState& state, const AeroParams& params) {
    const Eigen::Vector2d vRel = relativeVelocity(state, params);
    if (vRel.norm() < kEpsilon) {
        return 0.0;
    }

    const double gamma = std::atan2(vRel.y(), vRel.x());
    return wrapAngle(state.theta - gamma);
}

Eigen::Vector2d dragForce(const AeroState& state, const AeroParams& params) {
    const Eigen::Vector2d vRel = relativeVelocity(state, params);
    const double speed = vRel.norm();
    if (speed < kEpsilon || params.sref <= 0.0) {
        return Eigen::Vector2d::Zero();
    }

    const double dragMagnitude = dynamicPressure(speed, params.airDensity) * params.sref * params.cd;
    return -dragMagnitude * vRel.normalized();
}

Eigen::Vector2d normalForce(const AeroState& state, const AeroParams& params) {
    const Eigen::Vector2d vRel = relativeVelocity(state, params);
    const double speed = vRel.norm();
    if (speed < kEpsilon || params.sref <= 0.0) {
        return Eigen::Vector2d::Zero();
    }

    const double alpha = angleOfAttack(state, params);
    const double normalMagnitude = dynamicPressure(speed, params.airDensity) * params.sref * params.cna * alpha;
    return normalMagnitude * normalAxis(state.theta);
}

AeroForces aerodynamicForces(const AeroState& state, const AeroParams& params) {
    AeroForces result;
    const Eigen::Vector2d vRel = relativeVelocity(state, params);
    const double speed = vRel.norm();
    if (speed < kEpsilon || params.sref <= 0.0) {
        return result;
    }

    result.alpha = angleOfAttack(state, params);
    result.mach = params.speedOfSound > kEpsilon ? speed / params.speedOfSound : 0.0;
    result.drag = dragForce(state, params);
    result.normal = normalForce(state, params);
    result.total = result.drag + result.normal;

    const double leverArm = params.cp - params.cg;
    const Eigen::Vector2d rCpFromCg = -leverArm * bodyAxis(state.theta);
    result.pitchMoment = cross2(rCpFromCg, result.total);
    return result;
}

} // namespace aero2d
