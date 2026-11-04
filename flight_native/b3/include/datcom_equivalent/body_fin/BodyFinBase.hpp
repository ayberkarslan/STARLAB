#pragma once

#include <string>

#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent::body_fin {

struct BodyFinBaseResult {
    double kwBase = 0.0;
    double kb = 0.0;
    double kkw = 0.0;
    double kkb = 0.0;
    double rawXcpBw = 0.0;
    double rawXcpWb = 0.0;
    double xcpBw = 0.0;
    double xcpWb = 0.0;
    double eta = 0.0;
    double localBodyRadiusM = 0.0;
    std::string kernelFamily;
};

// Native raw-input owner for the supported sharp-cone plus cylindrical-body,
// one HEX FINSET contract.  Outputs preserve the recovered coordinator's
// REAL*4 state and caller normalization; no aerodynamic coefficients enter.
BodyFinBaseResult evaluateBodyFinBaseInterference(
    const BodyGeometry&, const FinGeometry&, const FlightCondition&,
    const ReferenceCondition&, double planformSlopePerRadian);

}  // namespace datcom_equivalent::body_fin
