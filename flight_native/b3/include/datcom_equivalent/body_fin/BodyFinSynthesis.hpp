#pragma once

#include <optional>

namespace datcom_equivalent::body_fin {

struct BodyFinCoefficientState {
    double cn = 0.0;
    double cm = 0.0;
    double ca = 0.0;
};

struct BodyFinSynthesisResult {
    BodyFinCoefficientState finInBody;
    BodyFinCoefficientState bodyCarryover;
    BodyFinCoefficientState total;
    double interactionCn = 0.0;
    double interactionCm = 0.0;
    std::optional<double> effectiveCp;
};

// Recovered first-FINSET CN/CM/CA carryover composition.  Direct-moment and
// axial carryover channels are retained explicitly for the owner boundary;
// the supported raw static contract supplies zero for all three.
BodyFinSynthesisResult synthesizeBodyFinStatic(
    const BodyFinCoefficientState& bodyAlone,
    const BodyFinCoefficientState& isolatedFin,
    double kw, double kb, double xcpWb, double xcpBw, double finInBodyCn,
    double finInBodyCa = 0.0, double finInBodyDirectCm = 0.0,
    double carryoverDirectCm = 0.0, double carryoverCa = 0.0);

}  // namespace datcom_equivalent::body_fin
