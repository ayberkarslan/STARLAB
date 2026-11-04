#pragma once

#include <Eigen/Dense>

#include "aero/DragBreakdown.hpp"
#include "aero/FlightCondition.hpp"

namespace FlightForces {

inline double dynamicPressure(const Eigen::Vector3d& relativeVelocity,
                              const FlightCondition& fc) {
    return fc.qbar > 0.0
        ? fc.qbar
        : 0.5 * fc.rho * relativeVelocity.squaredNorm();
}

inline double normalForceMagnitude(const Eigen::Vector3d& relativeVelocity,
                                   const FlightCondition& fc,
                                   double normalForceCoefficient) {
    if (relativeVelocity.squaredNorm() <= 1e-18
        || fc.referenceArea <= 0.0) {
        return 0.0;
    }
    return dynamicPressure(relativeVelocity, fc)
        * fc.referenceArea * normalForceCoefficient;
}

inline Eigen::Vector3d dragForce(const Eigen::Vector3d& relativeVelocity, const FlightCondition& fc, const DragBreakdown& drag) {
    const double speed = relativeVelocity.norm();
    if (speed <= 1e-9 || fc.referenceArea <= 0.0) {
        return Eigen::Vector3d::Zero();
    }

    const double qbar = dynamicPressure(relativeVelocity, fc);
    const double magnitude = qbar * fc.referenceArea * drag.total();
    return -magnitude * relativeVelocity.normalized();
}

} // namespace FlightForces
