#pragma once

// Faithful port of the sharp-cone + constant-diameter-cylinder BODY static
// normal-force/pitching-moment owner for stored REAL*4 Mach <= 1.2, read
// directly from:
//   third_party/datcom_equivalent_oracle/datcom_equivalent/body/normal.py
//     (evaluate_axisymmetric_body_static_normal and its helpers)
//
// M > 1.2 (Van Dyke Hybrid / SOSE) is out of scope for this owner and is
// reported as NotYetImplemented, matching the oracle's own
// `if mach > f32(1.2): raise UnsupportedBodyGeometryError(...)` fail-closed
// behavior for that route -- not silently routed anywhere.
//
// This owner reuses the already-validated CrossflowCdc.hpp CDC port for
// both the BDWK (selector=0.0) and viscous (selector=1.0 iff NATURAL,
// else 0.0) crossflow-drag-coefficient inputs; it does not reimplement any
// CDC equation.

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent::body {

// Mirrors normal.py's UnsupportedBodyGeometryError(NotImplementedError):
// a BODY configuration or Mach regime this owner has not (yet, or ever,
// for M>1.2 pending the Hybrid/SOSE port) recovered. Distinct from
// datcom_equivalent::UnsupportedGeometry (the top-level request-validity
// gate's rejection of a genuinely malformed geometry) -- this exception
// means "recognized but not implemented here", not "invalid input".
class BodyMethodNotImplemented : public std::runtime_error {
public:
    explicit BodyMethodNotImplemented(const std::string& what) : std::runtime_error(what) {}
};

struct BodyStaticNormalResult {
    double potentialCn = 0.0;
    double viscousCn = 0.0;
    double cnTotal = 0.0;

    double potentialCm = 0.0;
    double viscousCm = 0.0;
    double cmTotal = 0.0;

    // Selector-zero (BDWK) CDC, exposed on this result exactly as the
    // oracle's BodyStaticNormalResult.bdwk does. NOT the selector-one
    // viscous-CN/CM CDC (a distinct value; both are computed internally
    // but only BDWK's is carried on the result, matching the oracle).
    double bdwk = 0.0;

    // CM/CN in reference lengths; unset when cnTotal == 0 (the native
    // printed ratio is undefined there, per the oracle's own warning).
    std::optional<double> bodyCp;

    std::string activeMethod;
    bool methodFallback = false;
    std::string validity;
    std::vector<std::string> warnings;
};

// Evaluate the closed sharp-CONE + constant-diameter-cylinder, M<=1.2
// BODY subroute. `body` describes only nose length / diameter / center
// length / nose origin (sharp cone + constant diameter is guaranteed by
// datcom_equivalent::BodyGeometry's shape, matching how the oracle's own
// nose_type=="CONE"/bluntness==0/center_diameter==diameter checks reduce
// to no-ops once geometry is already constrained to that shape).
//
// Throws BodyMethodNotImplemented for flight.mach > 1.2 (Hybrid/SOSE not
// ported in this increment). Throws datcom_equivalent::UnsupportedGeometry
// for a non-positive nose length/diameter or negative center length
// (mirrors cone_cylinder_crossflow_geometry's `raise ValueError`).
BodyStaticNormalResult evaluateAxisymmetricBodyStaticNormal(const BodyGeometry& body,
                                                              const FlightCondition& flight,
                                                              const ReferenceCondition& reference,
                                                              double alphaDeg);

}  // namespace datcom_equivalent::body
