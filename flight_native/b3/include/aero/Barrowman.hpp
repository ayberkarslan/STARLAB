#pragma once

#include <cmath>

namespace Barrowman {

inline double transitionCna(
    double foreRadius,
    double aftRadius,
    double referenceRadius
) noexcept {
    if (referenceRadius <= 0.0) {
        return 0.0;
    }
    return 2.0 * (
        aftRadius * aftRadius - foreRadius * foreRadius
    ) / (referenceRadius * referenceRadius);
}

inline double transitionCpLocal(
    double foreRadius,
    double aftRadius,
    double length
) noexcept {
    const double radiusSum = foreRadius + aftRadius;
    if (length <= 0.0 || radiusSum <= 0.0) {
        return 0.0;
    }

    // Classical Barrowman conical-transition CP.  Unlike a direct
    // volume/area-difference quotient, this remains finite as the two
    // radii approach each other.
    return length * (foreRadius + 2.0 * aftRadius)
        / (3.0 * radiusSum);
}

}  // namespace Barrowman
