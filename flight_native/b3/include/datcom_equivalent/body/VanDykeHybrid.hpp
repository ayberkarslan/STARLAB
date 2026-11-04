#pragma once

// Faithful port of the Van Dyke Hybrid entry-point glue and required SOSE
// fallback dispatch, read directly from:
//   third_party/datcom_equivalent_oracle/datcom_equivalent/body/hybrid.py
//     (_validate_supported_supersonic_envelope, _normalized_cone_cylinder_contour,
//      _evaluate_hybrid_cached, evaluate_van_dyke_hybrid_sharp_cone)
//
// SOSE is not a separate algorithm here: `_evaluate_hybrid_cached` calls
// HybridPressureMachine once at the Hybrid entry point (0x004A6520); if
// that signals failure, it prepares a second memory image and calls the
// SAME machine at the SOSE entry point (0x004F9E80) within the same
// operation ledger. Both paths are ported together in this one function,
// matching the oracle's own structure -- see VanDykeHybrid.cpp.

#include <cstdint>
#include <optional>
#include <string>

#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent::body {

// Alpha-independent scalar state produced by Hybrid/SOSE. Folded into
// alpha-dependent potential CN/CM by the BODY supersonic owner (mirrors
// how the oracle's BodyStaticNormalResult.potential_cn/_cm are folded from
// HybridScalarResult.potential_cn_scalar/_cm_scalar via _folded_potential
// in hybrid.py, distinct from -- but the same shape as -- normal.py's
// low-Mach folding).
struct HybridScalarResult {
    double pressureWaveCa0 = 0.0;
    double pressureWaveAlphaScalar = 0.0;
    double potentialCnScalar = 0.0;
    double potentialCmScalar = 0.0;

    bool hybridSuccess = false;
    std::uint32_t failureSentinel = 0;
    std::string actualMethod;  // "VAN_DYKE_HYBRID" or "SECOND_ORDER_SHOCK_EXPANSION"
    bool fallbackEntered = false;

    // Diagnostic state for first-divergence debugging (mirrors
    // HybridScalarResult's diagnostic fields in the oracle).
    int contourStationCount = 0;
    std::size_t instructionCount = 0;
};

// Mirrors hybrid.py's _validate_supported_supersonic_envelope. Throws
// BodyMethodNotImplemented (see BodyNormal.hpp) outside the native-tested
// sharp-cone/cylinder supersonic envelope. `alphaDeg` is checked only when
// provided (mirrors the Python default `alpha_deg: float | None = None`
// -- the top-level BODY dispatcher passes it, evaluateVanDykeHybridSharpCone's
// own internal re-validation call does not, exactly as in hybrid.py).
void validateSupportedSupersonicEnvelope(const BodyGeometry& body, const FlightCondition& flight,
                                          const ReferenceCondition& reference,
                                          std::optional<double> alphaDeg = std::nullopt);

// Mirrors evaluate_van_dyke_hybrid_sharp_cone. Note: the oracle's
// `hybrid_reynolds_operand` flight-condition override key (a direct
// ABI-validation development hook, never part of the public raw request
// contract) is not exposed here -- this always takes the
// reynolds_per_m/mach production path.
HybridScalarResult evaluateVanDykeHybridSharpCone(const BodyGeometry& body,
                                                    const FlightCondition& flight,
                                                    const ReferenceCondition& reference);

}  // namespace datcom_equivalent::body
