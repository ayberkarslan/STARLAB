#pragma once

// Faithful port of DATCOM Equivalent `sub_00428D68` (BODY crossflow drag
// coefficient, CDC), read directly from:
//   third_party/datcom_equivalent_oracle/datcom_equivalent/body/crossflow_cdc.py
//
// Two call sites exist in the oracle's production BODY assembler
// (datcom_equivalent/body/normal.py::evaluate_axisymmetric_body_static_normal),
// and both are ported here, matching the oracle's own semantic split
// (see the docstring on `selector` below -- this file preserves that split
// rather than merging it, per instruction):
//
//   - BDWK (BODY-FIN interference factor): always selector = 0.0.
//   - viscous CN/CM: selector = 1.0 when boundary_layer == "NATURAL",
//     else 0.0 (i.e. it reduces to the same non-one path as BDWK for a
//     turbulent boundary layer -- this is the oracle's own behavior, not
//     an approximation introduced here).
//
// This module does not decide which selector to pass for either purpose;
// callers (a later BODY-normal/moment port) own that decision, exactly as
// normal.py's evaluate_axisymmetric_body_static_normal does.

#include <string>

#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent::body {

struct CrossflowCdcResult {
    double cdc = 0.0;
    double selector = 0.0;
    double crossflowMach = 0.0;
    double reynoldsOperand = 0.0;
    double reynoldsCrossflow = 0.0;
    double geometryScale = 0.0;
    std::string activeBranch;
    std::string validity;
};

// Direct ABI-level entry point. Mirrors
// crossflow_cdc.py::evaluate_crossflow_cdc_operands, itself documented
// there as "used only for binary boundary validation" -- i.e. this is the
// right entry point for synthetic branch-coverage cases, not just an
// internal helper.
CrossflowCdcResult evaluateCrossflowCdcOperands(double selector,
                                                 double alphaDeg,
                                                 double reynoldsPerInternalLength,
                                                 double geometryScale,
                                                 double mach);

// Public raw-geometry entry point. Mirrors
// crossflow_cdc.py::evaluate_body_crossflow_cdc. `body` must describe a
// sharp cone (bluntness 0) plus constant-diameter cylinder, which
// datcom_equivalent::BodyGeometry already guarantees by construction (single
// diameterM field for both nose base and center body) -- the oracle's
// separate nose_type/bluntness/center_diameter-mismatch checks reduce to
// the BodyGeometry positivity checks below.
//
// Throws datcom_equivalent::UnsupportedGeometry (see Validity.hpp) for
// non-positive nose length / diameter, or negative center length --
// exactly the oracle's own `raise ValueError("invalid cone/cylinder
// geometry")` condition, ported to this repo's validity exception
// taxonomy rather than a bare exception.
CrossflowCdcResult evaluateBodyCrossflowCdc(double selector,
                                             const BodyGeometry& body,
                                             double reynoldsPerM,
                                             double mach,
                                             double alphaDeg);

}  // namespace datcom_equivalent::body
