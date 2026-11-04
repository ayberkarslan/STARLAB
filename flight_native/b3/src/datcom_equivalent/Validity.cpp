#include "datcom_equivalent/Validity.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace datcom_equivalent {

namespace {

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return std::toupper(c); });
    return s;
}

}  // namespace

void validateRequest(const DatcomEquivalentRequest& request) {
    // request.py: DatcomEquivalentRequest.evaluate() gates beta/control/cant
    // before delegating to the provider.
    if (request.betaDeg != 0.0 || request.controlDeflectionDeg != 0.0 ||
        request.finCantDeg != 0.0) {
        throw UnsupportedConfiguration(
            "beta, control deflection, and fin cant must all be zero");
    }

    // validity.py: require_single_finset_static_contract, exact source order.
    const double diameter = request.body.diameterM;
    const double noseLength = request.body.noseLengthM;
    const double centerLength = request.body.centerLengthM;
    const double mach = request.flight.mach;
    const std::string boundaryLayer = toUpper(request.flight.boundaryLayer);
    const std::string family = toUpper(request.fin.sectionFamily);

    if (diameter <= 0.0 || noseLength <= 0.0 || centerLength <= 0.0) {
        throw UnsupportedGeometry(
            "sharp BODY nose, cylinder length, and diameter must be positive");
    }
    if (request.body.noseFamily == BodyNoseFamily::TangentOgive
        && noseLength < diameter / 2.0) {
        throw UnsupportedGeometry("tangent-ogive nose requires LNOSE >= DNOSE/2");
    }
    if (family != "HEX") {
        throw UnsupportedGeometry(
            "only HEX FIN planforms are supported end-to-end; ARC is fail-closed");
    }
    if (request.fin.npanel != 3 && request.fin.npanel != 4 && request.fin.npanel != 6) {
        throw UnsupportedConfiguration(
            "only native symmetric NPANEL=3, 4, or 6 layouts are supported");
    }
    if (boundaryLayer != "TURB" && boundaryLayer != "NATURAL") {
        throw UnsupportedFlightCondition("boundary_layer must be TURB or NATURAL");
    }
    const double maximumMach = request.body.noseFamily == BodyNoseFamily::TangentOgive ? 1.2 : 5.0;
    if (!(mach >= 0.2 && mach <= maximumMach)) {
        throw UnsupportedFlightCondition(
            request.body.noseFamily == BodyNoseFamily::TangentOgive
                ? "the tangent-ogive SUBTRANS envelope is 0.2 <= Mach <= 1.2"
                : "the public BODY axial envelope is 0.2 <= Mach <= 5.0");
    }
    if (std::abs(request.alphaDeg) > 10.0) {
        throw UnsupportedFlightCondition(
            "the recovered static-longitudinal alpha envelope is -10 to +10 degrees");
    }
    if (!(diameter >= 0.05 && diameter <= 0.4)) {
        throw UnsupportedGeometry(
            "the validated public BODY diameter envelope is 0.05 to 0.4 m");
    }
    if (!(request.flight.reynoldsPerM > 0.0)) {
        throw UnsupportedFlightCondition("unit Reynolds must be positive in 1/m");
    }
}

}  // namespace datcom_equivalent
