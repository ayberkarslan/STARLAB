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

struct BusemannCoefficients {
    double k1 = 0.0;
    double k2 = 0.0;
    double k3 = 0.0;
    double kStar = 0.0; // Upstream MAPLEAF sets Kstar = 0
};

/**
 * @brief Third-order Busemann supersonic expansion coefficients.
 * References: Niskanen Eqns 3.41-3.44, Barrowman Appendix A (2a-2c).
 * Note: Kstar is explicitly set to 0 to maintain exact parity with upstream MAPLEAF.
 */
inline BusemannCoefficients getBusemannCoefficients(double mach, double gamma = kGamma) {
    BusemannCoefficients coeffs;
    if (mach <= 1.0) {
        return coeffs;
    }

    const double beta = std::sqrt(mach * mach - 1.0);
    const double beta2 = beta * beta;
    const double beta4 = beta2 * beta2;
    const double beta7 = beta4 * beta2 * beta;
    const double mach2 = mach * mach;
    const double mach4 = mach2 * mach2;
    const double mach6 = mach4 * mach2;
    const double mach8 = mach4 * mach4;

    coeffs.k1 = 2.0 / beta;
    coeffs.k2 = ((gamma + 1.0) * mach4 - 4.0 * beta2) / (4.0 * beta4);
    coeffs.k3 = ((gamma + 1.0) * mach8 +
                 (2.0 * gamma * gamma - 7.0 * gamma - 5.0) * mach6 +
                 10.0 * (gamma + 1.0) * mach4 -
                 12.0 * mach2 + 8.0) / (6.0 * beta7);
    coeffs.kStar = 0.0; // Upstream limitation: Kstar set to 0

    return coeffs;
}

/**
 * @brief Supersonic fin slice force (excluding dynamic pressure).
 * Niskanen Eqns 3.41-3.44, CythonFinFunctions.pyx.
 */
inline double getFinSliceForce_Supersonic(
    double k1, double k2, double k3, double kStar,
    double sliceAOA, double sliceArea
) {
    const double kStarTop = 0.0;
    const double kStarBottom = 0.0;

    const double topCp = sliceAOA * (k1 + k2 * sliceAOA + k3 * sliceAOA * sliceAOA - kStarTop * sliceAOA * sliceAOA);
    const double negAoa = -sliceAOA;
    const double bottomCp = negAoa * (k1 + k2 * negAoa + k3 * negAoa * negAoa - kStarBottom * negAoa * negAoa);

    return (topCp - bottomCp) * sliceArea;
}

/**
 * @brief Slice-level Mach cone correction factor.
 *
 * Slices fully outside the Mach cone receive factor 1.0.
 * Slices fully inside the Mach cone receive factor 0.5.
 * Slices partially inside interpolate linearly: fractionOut + 0.5 * fractionIn.
 */
inline double calculateMachConeCorrection(double machConeZ, double sliceLeZ, double sliceLength) {
    if (sliceLength <= 1.0e-12) return 1.0;
    const double lengthOutOfMachCone = std::abs(machConeZ - sliceLeZ);
    const double fractionOut = std::clamp(lengthOutOfMachCone / sliceLength, 0.0, 1.0);
    const double fractionIn = 1.0 - fractionOut;
    return fractionOut + 0.5 * fractionIn;
}

} // namespace RoketSiM::Mapleaf
