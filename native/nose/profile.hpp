#pragma once
#include <algorithm>
#include <cmath>
#include <string>
// RoketSiM src/cli/design_analysis.hpp::noseRadius; only parameter transport changed.
namespace starlab_nose {
constexpr double kPi = 3.14159265358979323846;
inline double noseRadius(
    const std::string& shape,
    double z,
    double length,
    double baseRadius,
    double parameter
) {
    if (length <= 0.0) return 0.0;
    const double x = std::clamp(z / length, 0.0, 1.0);
    if (shape == "Conical") return baseRadius * x;
    if (shape == "Ellipsoid") {
        return baseRadius * std::sqrt(std::max(0.0, 1.0 - (1.0 - x) * (1.0 - x)));
    }
    if (shape == "Parabolic") {
        const double k = parameter;
        return baseRadius * (2.0 * x - k * x * x) / std::max(2.0 - k, 1e-9);
    }
    if (shape == "Power") {
        return baseRadius * std::pow(x, parameter);
    }
    if (shape == "Haack") {
        const double theta = std::acos(std::clamp(1.0 - 2.0 * x, -1.0, 1.0));
        const double c = parameter;
        const double inside = std::max(
            theta - 0.5 * std::sin(2.0 * theta)
                + c * std::pow(std::sin(theta), 3.0),
            0.0
        );
        return baseRadius * std::sqrt(inside / kPi);
    }
    // Tangent ogive
    if (baseRadius <= 0.0) return 0.0;
    const double rho =
        (baseRadius * baseRadius + length * length) / (2.0 * baseRadius);
    return std::sqrt(std::max(
        rho * rho - (length - z) * (length - z),
        0.0
    )) + baseRadius - rho;
}
}
