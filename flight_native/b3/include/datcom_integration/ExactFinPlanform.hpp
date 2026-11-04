#pragma once

#include <array>

namespace RoketSiM::DatcomIntegration {

enum class FinPlanformSource { ConventionalScalars, FreeformSourceOutline };

// Integration-internal proof result. All lengths are SI; leading edges are
// absolute axial stations in the canonical nose-aft frame. No fitted geometry.
struct ExactFinPlanform {
    double rootChordM;
    double tipChordM;
    double spanM;
    std::array<double, 2> leadingEdgeM;
    FinPlanformSource source;
};

}  // namespace RoketSiM::DatcomIntegration
