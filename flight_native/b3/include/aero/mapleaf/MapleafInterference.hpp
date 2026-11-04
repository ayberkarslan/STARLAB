// Portions derived from MAPLEAF
// Copyright (c) 2020 Henry Stoldt, AERO-CORE, and contributors
// Licensed under the MIT License.
// Upstream source: https://github.com/henrystoldt/MAPLEAF
// Upstream commit: af970d3e8200832f5e70d537b15ad38dd74fa551

#pragma once

namespace RoketSiM::Mapleaf {

/**
 * @brief Body-on-fin interference factor based on Barrowman / Niskanen Eqn 3.56.
 *
 * Accounts for additional normal force generated on the fins due to the presence of the rocket body.
 *
 * @param bodyRadius Radius of rocket body tube at fin root [m]
 * @param span Fin span (height above body) [m]
 * @return double Factor 1 + R / (span + R)
 */
inline double calculateBodyOnFinInterferenceFactor(double bodyRadius, double span) {
    if (span + bodyRadius <= 1.0e-12) return 1.0;
    return 1.0 + bodyRadius / (span + bodyRadius);
}

/**
 * @brief Fin-number interference factor based on MIL-HDBK-762 / Niskanen Eqn 3.54.
 *
 * Accounts for aerodynamic reduction when large numbers of fins are clustered together.
 * Note: Fixes upstream MAPLEAF bug where self.finNumber was accessed instead of self.numFins.
 *
 * @param numFins Number of fins in the axisymmetric set
 * @return double Interference reduction multiplier
 */
inline double calculateFinNumberInterferenceFactor(int numFins) {
    if (numFins <= 4) {
        return 1.0;
    }
    switch (numFins) {
        case 5: return 0.948;
        case 6: return 0.913;
        case 7: return 0.854;
        case 8: return 0.810;
        default: return 0.750; // Clamped for N > 8
    }
}

/**
 * @brief Combined interference multiplier applied to normal force and moments.
 */
inline double calculateTotalInterferenceFactor(double bodyRadius, double span, int numFins) {
    return calculateBodyOnFinInterferenceFactor(bodyRadius, span) *
           calculateFinNumberInterferenceFactor(numFins);
}

} // namespace RoketSiM::Mapleaf
