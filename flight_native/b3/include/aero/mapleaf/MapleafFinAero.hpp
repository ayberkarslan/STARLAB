// Portions derived from MAPLEAF
// Copyright (c) 2020 Henry Stoldt, AERO-CORE, and contributors
// Licensed under the MIT License.
// Upstream source: https://github.com/henrystoldt/MAPLEAF
// Upstream commit: af970d3e8200832f5e70d537b15ad38dd74fa551

#pragma once

#include "MapleafTypes.hpp"
#include "MapleafFinGeometry.hpp"
#include "MapleafSupersonicFin.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace RoketSiM::Mapleaf {

/**
 * @brief Barrowman's equation for subsonic Fin CN_alpha (Eqn 3-6), based on Diederich and Göthert.
 * Version from Niskanen Eqn 3.37.
 *
 * @param span Fin span (height) [m]
 * @param planformArea Fin planform area [m^2]
 * @param beta Prandtl-Glauert compressibility factor sqrt(1 - M^2)
 * @param midChordSweepRad Mid-chord sweep angle [rad]
 * @return double Normal force derivative C_Nalpha in [1/rad]
 */
inline double getFinCnAlpha_Subsonic_Barrowman(
    double span, double planformArea, double beta, double midChordSweepRad
) {
    if (planformArea <= 1.0e-12 || span <= 1.0e-12) return 0.0;
    const double ar = 2.0 * span * span / planformArea; // Version from Niskanen Eqn 3.37
    const double cosSweep = std::cos(midChordSweepRad);
    if (std::abs(cosSweep) <= 1.0e-12) return 0.0;

    const double numer = 2.0 * kPi * ar;
    const double term = (beta * ar / cosSweep);
    const double denom = 2.0 + std::sqrt(4.0 + term * term);
    return numer / denom;
}

/**
 * @brief Computes local angle of attack of a fin slice including roll-induced velocity.
 * Niskanen / CythonFinFunctions.pyx.
 *
 * @param finSpanPosition Radial distance from rocket centerline to slice center [m]
 * @param airVelRelativeToFin Air velocity relative to fin root [m/s]
 * @param unitSpanTangentialAirVelocity Tangential air velocity per unit radius (-omega x u_span) [m/s per m]
 * @param finNormal Unit vector normal to the fin surface
 * @param stallAngleRad Maximum stall angle [rad]
 * @return double Local angle of attack in radians
 */
inline double getFinSliceAngleOfAttack(
    double finSpanPosition,
    const Vector3& airVelRelativeToFin,
    const Vector3& unitSpanTangentialAirVelocity,
    const Vector3& finNormal,
    double stallAngleRad
) {
    const Vector3 finVelocity = airVelRelativeToFin + unitSpanTangentialAirVelocity * finSpanPosition;
    const double finVelMag = finVelocity.length();

    double fsaOa = 0.0;
    if (finVelMag >= 0.1) {
        double body = finNormal.dot(finVelocity) * (1.0 / finVelMag);
        if (std::abs(body - 1.0) < 1.0e-14) body = 1.0;
        if (std::abs(body + 1.0) < 1.0e-14) body = -1.0;
        body = std::clamp(body, -1.0, 1.0);
        fsaOa = std::asin(body);
    }

    // Limit angle of attack magnitude to stall angle
    if (std::abs(fsaOa) > stallAngleRad && stallAngleRad > 1.0e-12) {
        fsaOa = (fsaOa > 0.0 ? stallAngleRad : -stallAngleRad);
    }

    return fsaOa;
}

struct FinForceMomentResult {
    double normalForceMagnitude = 0.0; // Normal force coefficient sum (or force)
    double momentAboutCp = 0.0;        // Spanwise moment about spanwise CP
};

/**
 * @brief Subsonic fin spanwise force integration.
 * CythonFinFunctions.pyx: getSubsonicFinNormalForce.
 */
inline FinForceMomentResult getSubsonicFinNormalForce(
    const Vector3& airVelRelativeToFin,
    const Vector3& unitSpanTangentialAirVelocity,
    const Vector3& finNormal,
    double spanwiseCpRadius,
    double cnAlphaPerRad,
    double stallAngleRad,
    const std::vector<FinSlice>& slices
) {
    FinForceMomentResult res;
    for (const auto& slice : slices) {
        double sliceAoa = getFinSliceAngleOfAttack(
            slice.radius, airVelRelativeToFin, unitSpanTangentialAirVelocity, finNormal, stallAngleRad
        );
        double sliceForce = cnAlphaPerRad * sliceAoa * slice.area;
        res.normalForceMagnitude += sliceForce;
        res.momentAboutCp += sliceForce * (spanwiseCpRadius - slice.radius);
    }
    return res;
}

/**
 * @brief Supersonic fin spanwise force integration with Busemann expansion and Mach cone correction.
 * CythonFinFunctions.pyx: getSupersonicFinNormalForce.
 */
inline FinForceMomentResult getSupersonicFinNormalForce(
    const Vector3& airVelRelativeToFin,
    const Vector3& unitSpanTangentialAirVelocity,
    const Vector3& finNormal,
    const std::vector<double>& machConeEdgeZPos,
    double spanwiseCpRadius,
    double k1, double k2, double k3, double kStar,
    double stallAngleRad,
    const std::vector<FinSlice>& slices
) {
    FinForceMomentResult res;
    const std::size_t nSlices = slices.size();
    for (std::size_t i = 0; i < nSlices; ++i) {
        const auto& slice = slices[i];
        double sliceAoa = getFinSliceAngleOfAttack(
            slice.radius, airVelRelativeToFin, unitSpanTangentialAirVelocity, finNormal, stallAngleRad
        );
        double sliceForce = getFinSliceForce_Supersonic(k1, k2, k3, kStar, sliceAoa, slice.area);

        // Mach-cone correction
        double machConeZ = (i < machConeEdgeZPos.size()) ? machConeEdgeZPos[i] : slice.leadingEdgeZ;
        double correction = calculateMachConeCorrection(machConeZ, slice.leadingEdgeZ, slice.length);
        sliceForce *= correction;

        res.normalForceMagnitude += sliceForce;
        res.momentAboutCp += sliceForce * (spanwiseCpRadius - slice.radius);
    }
    return res;
}

/**
 * @brief Cubic Hermite interpolation for transonic normal force blending (M in [0.8, 1.4]).
 * MAPLEAF / OpenRocket empirical correlation method.
 */
inline double cubicHermiteInterp(
    double x, double x1, double x2,
    double f1, double f2, double f1_plus_dx, double f2_plus_dx, double dx
) {
    const double df1 = (f1_plus_dx - f1) / dx;
    const double df2 = (f2_plus_dx - f2) / dx;

    const double t = std::clamp((x - x1) / (x2 - x1), 0.0, 1.0);
    const double t2 = t * t;
    const double t3 = t2 * t;

    const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    const double h10 = t3 - 2.0 * t2 + t;
    const double h01 = -2.0 * t3 + 3.0 * t2;
    const double h11 = t3 - t2;

    const double span = x2 - x1;
    return h00 * f1 + h10 * span * df1 + h01 * f2 + h11 * span * df2;
}

} // namespace RoketSiM::Mapleaf
