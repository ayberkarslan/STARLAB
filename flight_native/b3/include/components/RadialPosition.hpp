#pragma once

#include <cmath>
#include <stdexcept>

// Radial placement is independent from the existing axial placement.  The
// rocket axis is local +Z; angle zero lies on local +X.
enum class RadialPositionMode {
    Coaxial,
    FreeOffset,
    OnParentSurface,
};

struct RadialPosition {
    RadialPositionMode mode = RadialPositionMode::Coaxial;
    double offset = 0.0;
    double angleRad = 0.0;

    double resolvedRadius(double parentOuterRadius = 0.0) const {
        if (!std::isfinite(offset) || !std::isfinite(angleRad)) {
            throw std::invalid_argument("Radial placement values must be finite.");
        }
        if (mode == RadialPositionMode::Coaxial) {
            return 0.0;
        }
        const double radius = mode == RadialPositionMode::OnParentSurface
            ? parentOuterRadius + offset : offset;
        if (radius < 0.0) {
            throw std::invalid_argument("Resolved radial offset must be non-negative.");
        }
        return radius;
    }
};