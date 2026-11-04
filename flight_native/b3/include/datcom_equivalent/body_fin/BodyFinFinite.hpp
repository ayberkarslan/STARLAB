#pragma once

#include <string>
#include <vector>

#include "datcom_equivalent/Types.hpp"
#include "datcom_equivalent/body_fin/BodyFinBase.hpp"
#include "datcom_equivalent/fin/FinPreprocessing.hpp"

namespace datcom_equivalent::body_fin {

struct EquivalentPanelAngleResult {
    double equivalentAlphaDeg = 0.0;
    double bodyFlowAngleRad = 0.0;
    double etaTableValue = 0.0;
    double sweepGeometryScale = 0.0;
    std::string helperBranch;
};

struct BodyFinPanelState {
    int index = 0;
    double phiDeg = 0.0;
    double equivalentAlphaDeg = 0.0;
    double cnPanel = 0.0;
    EquivalentPanelAngleResult upwash;
};

struct BodyFinFiniteResult {
    double kwAlphaScale = 1.0;
    double kwAlpha = 0.0;
    double eta = 0.0;
    double bdwk = 0.0;
    double finInBodyCn = 0.0;
    std::vector<BodyFinPanelState> panels;
    std::string activeUpwashMethod;
};

// Exact REAL*4 finite-alpha attenuation for the Stage-3 KW base factor.
double evaluateKwAlphaScale(double mach, double alphaDeg, double localBodyRadiusM,
                            double exposedSemispanM, double kwBase);

// Raw first-FINSET BODY-present normal-force producer.  The body operand is
// the selector-zero BODY crossflow CDC (BDWK), owned by the BODY path.
BodyFinFiniteResult evaluateBodyFinFiniteAlpha(
    const BodyGeometry&, const FinGeometry&, const FlightCondition&,
    const ReferenceCondition&, const fin::FinPreprocessingState&,
    const BodyFinBaseResult&, double alphaDeg, double bdwk);

}  // namespace datcom_equivalent::body_fin
