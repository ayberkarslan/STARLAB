// Portions derived from MAPLEAF
// Copyright (c) 2020 Henry Stoldt, AERO-CORE, and contributors
// Licensed under the MIT License.
// Upstream source: https://github.com/henrystoldt/MAPLEAF
// Upstream commit: af970d3e8200832f5e70d537b15ad38dd74fa551

#pragma once

#include "MapleafTypes.hpp"
#include <vector>
#include <cmath>

namespace RoketSiM::Mapleaf {

struct SingleFinOrientation {
    int finIndex = 0;
    double angleDeg = 0.0;
    Vector3 spanwiseDirection{1.0, 0.0, 0.0};     // Unit vector along fin span outward from body
    Vector3 undeflectedFinNormal{0.0, -1.0, 0.0}; // Unit normal vector (X/Y plane)
    Vector3 cpSpanwisePosition{0.0, 0.0, 0.0};    // Position of CP along span
};

class MapleafFinGeometry {
public:
    int numFins = 4;
    double firstFinAngleDeg = 0.0;
    double finCantAngleDeg = 0.0;
    double sweepAngleRad = 0.0;     // Leading edge sweep [rad]
    double rootChord = 0.0;         // Root chord length [m]
    double tipChord = 0.0;          // Tip chord length [m]
    double span = 0.0;              // Fin span (height) [m]
    double thickness = 0.0;         // Fin maximum thickness [m]
    double bodyRadius = 0.0;        // Body tube radius at fin attachment [m]
    double surfaceRoughness = 0.0;  // Surface roughness [m]
    int numSlices = 40;             // Number of spanwise integration slices

    // Precomputed geometric properties
    double planformArea = 0.0;
    double wettedArea = 0.0;
    double aspectRatio = 0.0;
    double stallAngleRad = 0.0;
    double trailingEdgeSweepRad = 0.0;
    double midChordSweepRad = 0.0;
    double dZ_dSpan_LE = 0.0;
    double dZ_dSpan_TE = 0.0;
    double macLength = 0.0;
    double macYPos = 0.0;
    double xMacLeadingEdge = 0.0;

    // Spanwise slices for force integration
    std::vector<FinSlice> slices;

    // Single fin orientations
    std::vector<SingleFinOrientation> orientations;

    MapleafFinGeometry() = default;

    void precompute(int sliceCount = 40);

    double getChord(double y) const {
        if (span <= 1.0e-12) return rootChord;
        return (tipChord - rootChord) * (y / span) + rootChord;
    }

    // Analytical MAC formulas for verification
    double getAnalyticalMACLength() const {
        if (rootChord + tipChord <= 1.0e-12) return 0.0;
        return (2.0 / 3.0) * (rootChord + tipChord - (rootChord * tipChord) / (rootChord + tipChord));
    }

    double getAnalyticalMACYPos() const {
        if (rootChord + tipChord <= 1.0e-12) return 0.0;
        return (span / 3.0) * ((rootChord + 2.0 * tipChord) / (rootChord + tipChord));
    }

    double getAnalyticalXMACLeadingEdge() const {
        return getAnalyticalMACYPos() * dZ_dSpan_LE;
    }
};

} // namespace RoketSiM::Mapleaf
