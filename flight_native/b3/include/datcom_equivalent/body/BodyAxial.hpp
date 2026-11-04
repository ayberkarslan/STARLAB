#pragma once

#include <string>
#include <vector>

#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent::body {

struct BodyStaticAxialResult {
    double skinFrictionCa = 0.0;
    double pressureWaveCa = 0.0;
    double baseCa = 0.0;
    double finiteAlphaIncrementCa = 0.0;
    double caTotal = 0.0;
    double skinFrictionCa0 = 0.0;
    double pressureWaveCa0 = 0.0;
    double baseCa0 = 0.0;
    std::string activePressureMethod;
    std::string activeNormalMethod;
    bool normalMethodFallback = false;
    std::string validity;
    std::vector<std::string> warnings;
};

// Faithful sharp-cone + constant-diameter-cylinder BODY axial owner.  CA-BASE
// is reported but deliberately excluded from caTotal, matching the recovered
// native composition.
BodyStaticAxialResult evaluateAxisymmetricBodyStaticAxial(const BodyGeometry& body,
                                                           const FlightCondition& flight,
                                                           const ReferenceCondition& reference,
                                                           double alphaDeg);

}  // namespace datcom_equivalent::body
