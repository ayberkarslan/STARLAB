#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include <Eigen/Dense>

enum class MotorClusterLayout {
    Single,
    Linear,
    Ring,
    Grid,
};

struct MotorClusterConfiguration {
    MotorClusterLayout layout = MotorClusterLayout::Single;
    int count = 1;
    double radius = 0.0;
    double rotationRad = 0.0;

    void validate() const {
        if (count < 1 || count > 64) {
            throw std::invalid_argument("Motor cluster count must be between 1 and 64.");
        }
        if (!std::isfinite(radius) || radius < 0.0
            || !std::isfinite(rotationRad)) {
            throw std::invalid_argument("Motor cluster geometry must be finite and non-negative.");
        }
        if (count > 1 && radius <= 0.0) {
            throw std::invalid_argument("A multi-motor cluster needs a positive radius.");
        }
        if (count > 1 && layout == MotorClusterLayout::Single) {
            throw std::invalid_argument(
                "A multi-motor cluster must use linear, ring, or grid layout.");
        }
    }
};

inline MotorClusterLayout motorClusterLayoutFromString(const std::string& value) {
    if (value == "linear") return MotorClusterLayout::Linear;
    if (value == "ring" || (value.size() > 5
        && value.compare(value.size() - 5, 5, "-ring") == 0)) {
        return MotorClusterLayout::Ring;
    }
    if (value == "grid") return MotorClusterLayout::Grid;
    return MotorClusterLayout::Single;
}

inline const char* motorClusterLayoutName(MotorClusterLayout layout) {
    switch (layout) {
    case MotorClusterLayout::Linear: return "linear";
    case MotorClusterLayout::Ring: return "ring";
    case MotorClusterLayout::Grid: return "grid";
    default: return "single";
    }
}

inline std::vector<Eigen::Vector2d> motorClusterPositions(
    const MotorClusterConfiguration& configuration
) {
    configuration.validate();
    if (configuration.count == 1) {
        return {Eigen::Vector2d::Zero()};
    }

    std::vector<Eigen::Vector2d> points;
    points.reserve(static_cast<std::size_t>(configuration.count));
    const double pi = std::acos(-1.0);
    if (configuration.layout == MotorClusterLayout::Grid) {
        const int columns = static_cast<int>(std::ceil(std::sqrt(
            static_cast<double>(configuration.count))));
        const int rows = static_cast<int>(std::ceil(
            static_cast<double>(configuration.count) / columns));
        const double halfX = std::max(0.5 * (columns - 1), 0.5);
        const double halfY = std::max(0.5 * (rows - 1), 0.5);
        const double scale = configuration.radius / std::hypot(halfX, halfY);
        for (int index = 0; index < configuration.count; ++index) {
            const int row = index / columns;
            const int column = index % columns;
            points.emplace_back(
                (column - 0.5 * (columns - 1)) * scale,
                (row - 0.5 * (rows - 1)) * scale);
        }
    } else if (configuration.layout == MotorClusterLayout::Linear) {
        const double denominator = std::max(configuration.count - 1, 1);
        for (int index = 0; index < configuration.count; ++index) {
            const double x = -configuration.radius
                + 2.0 * configuration.radius * index / denominator;
            points.emplace_back(x, 0.0);
        }
    } else {
        for (int index = 0; index < configuration.count; ++index) {
            const double angle = 2.0 * pi * index / configuration.count;
            points.emplace_back(
                configuration.radius * std::cos(angle),
                configuration.radius * std::sin(angle));
        }
    }

    // Incomplete grid rows are not inherently symmetric.  Recenter every
    // generated pattern so selecting a cluster never introduces a hidden CG
    // offset, then preserve radius as the maximum motor-center distance.
    Eigen::Vector2d centroid = Eigen::Vector2d::Zero();
    for (const auto& point : points) {
        centroid += point;
    }
    centroid /= static_cast<double>(points.size());
    double maximumRadius = 0.0;
    for (auto& point : points) {
        point -= centroid;
        maximumRadius = std::max(maximumRadius, point.norm());
    }
    if (maximumRadius > 0.0) {
        const double scale = configuration.radius / maximumRadius;
        for (auto& point : points) {
            point *= scale;
        }
    }

    const double cosine = std::cos(configuration.rotationRad);
    const double sine = std::sin(configuration.rotationRad);
    for (auto& point : points) {
        point = Eigen::Vector2d(
            cosine * point.x() - sine * point.y(),
            sine * point.x() + cosine * point.y());
    }
    return points;
}
