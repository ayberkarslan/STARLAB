#pragma once
#include <Eigen/Dense>

struct Transform {
    Eigen::Vector3d position = Eigen::Vector3d::Zero();
    Eigen::Matrix3d rotation = Eigen::Matrix3d::Identity();
};
