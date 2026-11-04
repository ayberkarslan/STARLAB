// Portions derived from MAPLEAF
// Copyright (c) 2020 Henry Stoldt, AERO-CORE, and contributors
// Licensed under the MIT License.
// Upstream source: https://github.com/henrystoldt/MAPLEAF
// Upstream commit: af970d3e8200832f5e70d537b15ad38dd74fa551

#pragma once

#include "MapleafTypes.hpp"
#include <algorithm>
#include <cmath>

namespace RoketSiM::Mapleaf {

/**
 * @brief Subsonic/supersonic skin friction compressibility correction factors.
 * Barrowman Eqns 4-9 to 4-13.
 */
inline double subsonicSkinFrictionCompressibility(double mach, bool smoothSurface) {
    if (smoothSurface) {
        return 1.0 - 0.10 * mach * mach;
    } else {
        return 1.0 - 0.12 * mach * mach;
    }
}

inline double supersonicSkinFrictionCompressibility(double mach, bool smoothSurface) {
    const double smoothTurbFactor = 1.0 / std::pow(1.0 + 0.15 * mach * mach, 0.58);
    if (smoothSurface) {
        return smoothTurbFactor;
    } else {
        const double roughTurbFactor = 1.0 / (1.0 + 0.18 * mach * mach);
        return std::max(smoothTurbFactor, roughTurbFactor);
    }
}

/**
 * @brief Flat-plate skin friction drag coefficient normalized by wetted area.
 * References: Barrowman (1967) pp. 43-48, Niskanen (2009) pp. 43-46.
 */
inline double getSkinFrictionCoefficient(
    double reynoldsNumber, double mach, double surfaceRoughness, double length,
    bool fullyTurbulent = true
) {
    if (reynoldsNumber <= 1.0e-12 || length <= 1.0e-12) return 0.0148;

    const bool smoothSurface = (surfaceRoughness <= 1.0e-12);
    const double criticalRe = smoothSurface ? 1.0e30 : (51.0 * std::pow(surfaceRoughness / length, -1.039));

    double cf = 0.0;
    if (reynoldsNumber > criticalRe) {
        // Roughness dominated
        cf = 0.032 * std::pow(surfaceRoughness / length, 0.2);
    } else if (fullyTurbulent) {
        if (reynoldsNumber < 1.0e4) {
            cf = 0.0148;
        } else {
            const double term = 1.5 * std::log(reynoldsNumber) - 5.6;
            cf = 1.0 / (term * term);
        }
    } else {
        // Laminar / Transitional
        if (reynoldsNumber < 1.0e4) {
            cf = 0.01328;
        } else if (reynoldsNumber < 5.0e5) {
            cf = 1.328 / std::sqrt(reynoldsNumber);
        } else {
            const double term = 1.5 * std::log(reynoldsNumber) - 5.6;
            cf = 1.0 / (term * term) - 1700.0 / reynoldsNumber;
        }
    }

    // Compressibility correction
    double compFactor = 1.0;
    if (mach < 0.9) {
        compFactor = subsonicSkinFrictionCompressibility(mach, smoothSurface);
    } else if (mach < 1.1) {
        double subFactor = subsonicSkinFrictionCompressibility(mach, smoothSurface);
        double supFactor = supersonicSkinFrictionCompressibility(mach, smoothSurface);
        compFactor = subFactor + (supFactor - subFactor) * (mach - 0.9) / 0.2;
    } else {
        compFactor = supersonicSkinFrictionCompressibility(mach, smoothSurface);
    }

    return cf * compFactor;
}

/**
 * @brief Base drag coefficient as function of Mach number.
 * MAPLEAF / Hoerner empirical base pressure correlation.
 */
inline double getBaseDragCoefficient(double mach) {
    if (mach < 1.0) {
        return 0.12 + 0.13 * mach * mach;
    } else {
        return 0.25 / mach;
    }
}

/**
 * @brief Conversion factor from total drag to axial force: cos(alpha_avg).
 */
inline double getDragToAxialForceFactor(double avgAoaRad) {
    return std::cos(avgAoaRad);
}

} // namespace RoketSiM::Mapleaf
