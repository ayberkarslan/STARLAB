#include "datcom_equivalent/body/BodySupersonic.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#include "datcom_equivalent/Validity.hpp"
#include "datcom_equivalent/body/CrossflowCdc.hpp"
#include "datcom_equivalent/body/VanDykeHybrid.hpp"
#include "datcom_equivalent/common/Float32.hpp"

namespace datcom_equivalent::body {

namespace {

using datcom_equivalent::common::f32;

constexpr double kDegreesPerRadianReal4 = 57.295780181884766;
// Must match the oracle's f32(1.2), not raw 1.2 -- see VanDykeHybrid.cpp
// for why this matters at the exact stored-REAL4 boundary.
constexpr double kMinSupersonicMach = static_cast<float>(1.2);

// hybrid.py has its own local _folded_potential, algebraically identical
// to normal.py's (same value, same f32 boundaries; only the negation's
// code shape differs -- `-value if alpha_deg<0 else value` vs an in-place
// `value = -value`). Kept as a separate local copy here rather than
// sharing BodyNormal.cpp's private helper, mirroring the oracle's own
// duplication across the two modules rather than inventing a shared
// abstraction the oracle doesn't have.
double foldedPotential(double scalar, double alphaDeg) {
    double magnitude = std::abs(f32(alphaDeg));
    if (magnitude >= f32(90.0)) {
        magnitude = f32(f32(180.0) - magnitude);
    }
    const double radians = magnitude / kDegreesPerRadianReal4;
    const double value = (f32(scalar) / 2.0) * std::sin(2.0 * radians) * std::cos(radians / 2.0);
    return f32(alphaDeg < 0.0 ? -value : value);
}

}  // namespace

BodyStaticNormalResult evaluateSupersonicBodyStaticNormal(const BodyGeometry& body,
                                                            const FlightCondition& flight,
                                                            const ReferenceCondition& reference,
                                                            double alphaDeg) {
    if (body.noseFamily != BodyNoseFamily::SharpCone) {
        throw BodyMethodNotImplemented(
            "tangent-ogive Mach>1.2 SUPPOT/HYBRID owner family is not implemented");
    }
    validateSupportedSupersonicEnvelope(body, flight, reference, alphaDeg);

    const double noseLength = body.noseLengthM;
    const double diameter = body.diameterM;
    const double centerLength = body.centerLengthM;
    const double noseOrigin = body.noseOriginM;

    const double sref = f32(reference.areaM2);
    const double lref = f32(reference.lengthM);
    const double xcg = f32(reference.xcgM);

    // area = cone_cylinder_crossflow_geometry(...).area (first of the 3
    // returned scalars; slenderness is not used on the supersonic path).
    if (std::min(noseLength, diameter) <= 0.0 || centerLength < 0.0) {
        throw UnsupportedGeometry("invalid cone/cylinder geometry");
    }
    const double area = f32(f32(0.5) * f32(noseLength) * f32(diameter) +
                             f32(centerLength) * f32(diameter));
    // centroid = cone_cylinder_projected_centroid(...)
    const double noseArea = f32(f32(0.5) * f32(noseLength) * f32(diameter));
    const double centerArea = f32(f32(centerLength) * f32(diameter));
    const double centroidNumerator =
        noseArea * (f32(2.0) * f32(noseLength) / f32(3.0)) +
        centerArea * (f32(noseLength) + f32(centerLength) / f32(2.0));
    const double centroid = f32(centroidNumerator / area);

    std::string boundaryLayer = flight.boundaryLayer;
    std::transform(boundaryLayer.begin(), boundaryLayer.end(), boundaryLayer.begin(),
                    [](unsigned char c) { return std::toupper(c); });
    const double viscousSelector = (boundaryLayer == "NATURAL") ? 1.0 : 0.0;
    const auto viscousCdc = evaluateBodyCrossflowCdc(viscousSelector, body, flight.reynoldsPerM,
                                                      flight.mach, alphaDeg);
    const auto bdwkCdc = evaluateBodyCrossflowCdc(0.0, body, flight.reynoldsPerM, flight.mach,
                                                   alphaDeg);

    const double sine = std::sin(f32(alphaDeg) / kDegreesPerRadianReal4);
    const double viscousCn = f32(viscousCdc.cdc * area / sref * sine * std::abs(sine));
    const double viscousCm = f32(viscousCdc.cdc * area * (xcg - f32(noseOrigin + centroid)) /
                                  lref * sine * std::abs(sine) / sref);

    const auto hybrid = evaluateVanDykeHybridSharpCone(body, flight, reference);
    const double potentialCn = foldedPotential(hybrid.potentialCnScalar, alphaDeg);
    const double potentialCm = foldedPotential(hybrid.potentialCmScalar, alphaDeg);

    const double cnTotal = f32(potentialCn + viscousCn);
    const double cmTotal = f32(potentialCm + viscousCm);

    BodyStaticNormalResult result;
    result.potentialCn = potentialCn;
    result.viscousCn = viscousCn;
    result.cnTotal = cnTotal;
    result.potentialCm = potentialCm;
    result.viscousCm = viscousCm;
    result.cmTotal = cmTotal;
    result.bdwk = bdwkCdc.cdc;
    result.bodyCp = (cnTotal == 0.0) ? std::nullopt : std::optional<double>(f32(cmTotal / cnTotal));
    result.activeMethod = hybrid.fallbackEntered
                               ? "SOSE_FALLBACK_TRANSLATED_SUB_004F9E80"
                               : "VAN_DYKE_HYBRID_TRANSLATED_SUB_004A1638_SUB_00535648";
    result.methodFallback = hybrid.fallbackEntered;
    result.validity = "SHARP_CONE_CONSTANT_DIAMETER_CYLINDER_HYBRID_WITH_REQUIRED_SOSE_FALLBACK";
    result.warnings = {"BODY axial force is excluded"};
    return result;
}

BodyStaticNormalResult evaluateBodyStaticNormalMoment(const BodyGeometry& body,
                                                        const FlightCondition& flight,
                                                        const ReferenceCondition& reference,
                                                        double alphaDeg) {
    if (f32(flight.mach) <= kMinSupersonicMach) {
        return evaluateAxisymmetricBodyStaticNormal(body, flight, reference, alphaDeg);
    }
    if (body.noseFamily != BodyNoseFamily::SharpCone) {
        throw BodyMethodNotImplemented(
            "tangent-ogive Mach>1.2 SUPPOT/HYBRID owner family is not implemented");
    }
    return evaluateSupersonicBodyStaticNormal(body, flight, reference, alphaDeg);
}

}  // namespace datcom_equivalent::body
