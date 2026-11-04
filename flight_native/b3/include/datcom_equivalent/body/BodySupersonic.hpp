#pragma once

// Faithful port of the supersonic (stored REAL*4 Mach > 1.2) sharp-cone
// BODY static normal-force/pitching-moment owner, read directly from:
//   third_party/datcom_equivalent_oracle/datcom_equivalent/body/hybrid.py
//     (evaluate_axisymmetric_body_static_normal, supersonic branch;
//      _folded_potential)
//
// Potential CN/CM come from Van Dyke Hybrid / required SOSE fallback
// (VanDykeHybrid.hpp). Viscous CN/CM and BDWK reuse the already-validated
// CDC owner (CrossflowCdc.hpp) -- no CDC equation is duplicated here,
// exactly as increment 3's low-Mach owner does.

#include "datcom_equivalent/body/BodyNormal.hpp"  // BodyStaticNormalResult, BodyMethodNotImplemented
#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent::body {

// Evaluate the sharp-CONE + constant-diameter-cylinder BODY subroute for
// stored REAL*4 Mach > 1.2 (through 5.0). Throws BodyMethodNotImplemented
// outside the native-tested supersonic envelope (Mach range, cone/center
// fineness, diameter, alpha, Sref/Lref positivity -- see
// VanDykeHybrid.hpp's validateSupportedSupersonicEnvelope).
BodyStaticNormalResult evaluateSupersonicBodyStaticNormal(const BodyGeometry& body,
                                                            const FlightCondition& flight,
                                                            const ReferenceCondition& reference,
                                                            double alphaDeg);

// Shared production dispatcher (task-requested public entry point):
// stored REAL*4 Mach <= 1.2 -> the already-validated
// evaluateAxisymmetricBodyStaticNormal (BodyNormal.hpp), unmodified;
// stored REAL*4 Mach > 1.2  -> evaluateSupersonicBodyStaticNormal above.
// Mirrors hybrid.py's own evaluate_axisymmetric_body_static_normal, whose
// first line is exactly this dispatch (`if mach <= MIN_SUPERSONIC_MACH_REAL4:
// return _frozen_low_mach(...)`).
BodyStaticNormalResult evaluateBodyStaticNormalMoment(const BodyGeometry& body,
                                                        const FlightCondition& flight,
                                                        const ReferenceCondition& reference,
                                                        double alphaDeg);

}  // namespace datcom_equivalent::body
