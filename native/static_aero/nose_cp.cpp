#include <algorithm>
#include <cmath>
#include "../export.hpp"

namespace {
constexpr double pi = 3.141592653589793238462643383279502884;
double tangentOgiveCpFromVolume(double length, double baseRadius) {
    if (length <= 0.0 || baseRadius <= 0.0) return 0.0;
    const double rho = (baseRadius * baseRadius + length * length) / (2.0 * baseRadius);
    constexpr int slices = 2000;
    const double dz = length / slices;
    double volume = 0.0;
    for (int i = 0; i < slices; ++i) {
        const double z = (i + 0.5) * dz;
        const double r = std::sqrt(std::max(rho * rho - (length - z) * (length - z), 0.0)) + baseRadius - rho;
        volume += pi * r * r * dz;
    }
    return length - volume / (pi * baseRadius * baseRadius);
}
}

STARLAB_EXPORT double static_nose_cp(
    double length, double radius, double profile, double,
    double, double, double overrideEnabled, double overrideValue
) {
    if (overrideEnabled != 0.0) return overrideValue;
    if (profile == 0.0) return 2.0 * length / 3.0;
    if (profile == 1.0) return tangentOgiveCpFromVolume(length, radius);
    return 0.5 * length;
}
