#pragma once
#include "sensors/Measurements.hpp"
#include <Eigen/Core>
#include <optional>
#include <vector>

namespace estimation {
struct InitialState {
    double timestamp = 0, altitude_m = 0, vertical_velocity_mps = 0, accel_bias_mps2 = 0;
    Eigen::Matrix3d covariance = Eigen::Vector3d(25,25,1).asDiagonal();
};
struct EstimatedState {
    double timestamp = 0;
    double altitude_m = 0, vertical_velocity_mps = 0;
    std::optional<double> accel_bias_mps2;
    std::optional<Eigen::Matrix3d> covariance; // ordering h, v, b; SI squared/cross units
    bool initialized = false, valid = false;
};
struct Innovation {
    double timestamp = 0, delivery_timestamp = 0;
    double residual_m = 0, covariance_m2 = 0, nis = 0;
};
class StateEstimator : public sensors::Estimator {
public:
    virtual void reset(const InitialState&) = 0;
    virtual EstimatedState state() const = 0;
    virtual std::vector<Innovation> innovations() const { return {}; }
};
}
