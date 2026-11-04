#pragma once

#include <Eigen/Dense>

struct MassProperties {
    double mass = 0.0;
    Eigen::Vector3d cgLocal = Eigen::Vector3d::Zero();
    Eigen::Matrix3d inertiaLocal = Eigen::Matrix3d::Zero();
};
