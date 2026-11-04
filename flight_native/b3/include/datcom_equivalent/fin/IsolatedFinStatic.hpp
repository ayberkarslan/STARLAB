#pragma once

#include <optional>
#include <string>

#include "datcom_equivalent/Types.hpp"
#include "datcom_equivalent/fin/FinPreprocessing.hpp"

namespace datcom_equivalent::fin {

struct IsolatedFinStaticResult {
    double ca0Panel = 0.0;
    double cnaPanelPerDegree = 0.0;
    double cnaPanelPerRadian = 0.0;
    double rawPlanformSlopePerDegree = 0.0;
    double cnLinear = 0.0;
    double cnNonlinear = 0.0;
    double cn = 0.0;
    double cmLinear = 0.0;
    double cmNonlinear = 0.0;
    double cm = 0.0;
    double cpLinearM = 0.0;
    double cpNonlinearM = 0.0;
    std::optional<double> cpM;
    std::string cnaMethod;
    std::string nonlinearMethod;
    std::string cpMethod;
};

// Canonical positive/negative-incidence response of one panel before the
// FINSET orientation assembly.  BODY-FIN upwash owns the panel-angle
// transform and natural-cubic loading curve; it must not reverse-engineer
// this state from the assembled free-air coefficient.
struct IsolatedFinPanelNormalForceResult {
    double cn = 0.0;
    double cnLinear = 0.0;
    double cnNonlinear = 0.0;
    double cnaPerDegree = 0.0;
};

// Native, raw-input owner.  Preprocessing stays separate and is passed only
// as validated state; callers never provide aerodynamic coefficients.
IsolatedFinStaticResult evaluateIsolatedFinStatic(
    const FinGeometry&, const FinPreprocessingState&, const FlightCondition&,
    const ReferenceCondition&, double alphaDeg);

IsolatedFinPanelNormalForceResult evaluateIsolatedFinPanelNormalForce(
    const FinGeometry&, const FinPreprocessingState&, const FlightCondition&,
    const ReferenceCondition&, double alphaDeg);

}  // namespace datcom_equivalent::fin
