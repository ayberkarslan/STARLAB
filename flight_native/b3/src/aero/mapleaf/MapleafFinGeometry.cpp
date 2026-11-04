// Portions derived from MAPLEAF
// Copyright (c) 2020 Henry Stoldt, AERO-CORE, and contributors
// Licensed under the MIT License.
// Upstream source: https://github.com/henrystoldt/MAPLEAF
// Upstream commit: af970d3e8200832f5e70d537b15ad38dd74fa551

#include "aero/mapleaf/MapleafFinGeometry.hpp"
#include <algorithm>
#include <cmath>

namespace RoketSiM::Mapleaf {

void MapleafFinGeometry::precompute(int sliceCount) {
    numSlices = (sliceCount > 0) ? sliceCount : 40;

    // Planform and aspect ratio
    planformArea = span * (rootChord + tipChord) / 2.0;
    wettedArea = 2.0 * planformArea;

    // Wing aspect ratio of reflected fin about root (Fleeman / Niskanen)
    aspectRatio = (planformArea > 1.0e-12) ? (4.0 * span * span / planformArea) : 0.0;

    // Stall angle from Hoerner 'Fluid Dynamic Lift' for low-aspect-ratio wings
    stallAngleRad = (15.0 * (1.0 + (aspectRatio > 1.0e-6 ? 1.1 / aspectRatio : 0.0))) * (kPi / 180.0);

    // Leading and trailing edge sweep angles
    const double leTipZ = span * std::tan(sweepAngleRad);
    const double teTipZ = tipChord + leTipZ - rootChord;

    if (std::abs(teTipZ) <= 1.0e-14) {
        trailingEdgeSweepRad = 0.0;
    } else if (teTipZ < 0.0) {
        trailingEdgeSweepRad = -std::atan2(std::abs(teTipZ), span);
    } else {
        trailingEdgeSweepRad = std::atan2(teTipZ, span);
    }

    midChordSweepRad = (sweepAngleRad + trailingEdgeSweepRad) / 2.0;
    dZ_dSpan_LE = std::tan(sweepAngleRad);
    dZ_dSpan_TE = std::tan(trailingEdgeSweepRad);

    // MAC properties via numerical integration (Niskanen Eqns 3.30-3.32)
    // High-resolution numerical integration steps (1 step per 0.1 mm or min 200 steps)
    const int macSteps = std::max(200, static_cast<int>(std::round(span / 0.0001)));
    const double macStepSize = (macSteps > 0) ? (span / macSteps) : 0.0;

    double macIntegralSum = 0.0;
    double macYIntegralSum = 0.0;
    double xMacLeIntegralSum = 0.0;

    for (int i = 0; i < macSteps; ++i) {
        double y = (i + 0.5) * macStepSize;
        double chord = getChord(y);
        double dArea = chord * macStepSize;

        macIntegralSum += chord * dArea;
        macYIntegralSum += y * dArea;
        xMacLeIntegralSum += (y * dZ_dSpan_LE) * dArea;
    }

    if (planformArea > 1.0e-12) {
        macLength = macIntegralSum / planformArea;
        macYPos = macYIntegralSum / planformArea;
        xMacLeadingEdge = xMacLeIntegralSum / planformArea;
    } else {
        macLength = rootChord;
        macYPos = span / 3.0;
        xMacLeadingEdge = 0.0;
    }

    // Split fin into spanwise slices for aerodynamic integration
    slices.clear();
    slices.reserve(numSlices);
    const double sliceStep = span / numSlices;

    for (int i = 0; i < numSlices; ++i) {
        double y = (i + 0.5) * sliceStep;
        FinSlice slice;
        slice.radius = y + bodyRadius;
        slice.length = getChord(y);
        slice.area = sliceStep * slice.length;
        slice.leadingEdgeZ = y * dZ_dSpan_LE;
        slices.push_back(slice);
    }

    // Fin orientations for N fins: deltaPhi = 360 deg / N, phi_i = phi_0 + i * deltaPhi
    orientations.clear();
    if (numFins > 0) {
        orientations.reserve(numFins);
        const double separationDeg = 360.0 / numFins;
        for (int i = 0; i < numFins; ++i) {
            double angleDeg = firstFinAngleDeg + i * separationDeg;
            double angleRad = angleDeg * (kPi / 180.0);

            SingleFinOrientation orient;
            orient.finIndex = i;
            orient.angleDeg = angleDeg;
            orient.spanwiseDirection = Vector3(std::cos(angleRad), std::sin(angleRad), 0.0);
            orient.undeflectedFinNormal = Vector3(orient.spanwiseDirection.y, -orient.spanwiseDirection.x, 0.0);
            orient.cpSpanwisePosition = orient.spanwiseDirection * (macYPos + bodyRadius);

            orientations.push_back(orient);
        }
    }
}

} // namespace RoketSiM::Mapleaf
