#pragma once

#include <Eigen/Dense>

namespace aero2d {

struct AeroState {
    double theta = 0.0;
    double vx = 0.0;
    double vz = 0.0;
    double altitude = 0.0;
};

struct AeroParams {
    double cd = 1.073;
    double cna = 0.0;
    double cp = 0.0;
    double cg = 0.0;
    double sref = 0.0;
    double airDensity = 1.225;
    double speedOfSound = 340.0;
    Eigen::Vector2d wind = Eigen::Vector2d::Zero();
};

struct AeroForces {
    Eigen::Vector2d drag = Eigen::Vector2d::Zero();
    Eigen::Vector2d normal = Eigen::Vector2d::Zero();
    Eigen::Vector2d total = Eigen::Vector2d::Zero();
    double alpha = 0.0;
    double mach = 0.0;
    double pitchMoment = 0.0;
};

double wrapAngle(double angle);
double angleOfAttack(const AeroState& state, const AeroParams& params);
Eigen::Vector2d dragForce(const AeroState& state, const AeroParams& params);
Eigen::Vector2d normalForce(const AeroState& state, const AeroParams& params);
AeroForces aerodynamicForces(const AeroState& state, const AeroParams& params);

} // namespace aero2d
