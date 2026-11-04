#pragma once

// Exact 0x004BD150 ascending-axis linear interpolation/extrapolation.
//
// Faithful port of crossflow_cdc.py::_linear (identical helper is inlined
// per-owner in several other oracle modules, e.g. body/normal.py's
// module-level `_linear`; ported once here since the behavior is byte-for-
// byte identical and the oracle's own common/tables.py hosts the shared
// table-access helpers this belongs alongside).
//
// Semantics (do not "simplify" this against e.g. std::lower_bound -- the
// scan order and off-by-one clamp below are exactly what the recovered
// binary does, including its extrapolation-by-continuing-the-nearest-
// segment behavior at both ends of the axis):
//
//   query < axis[0]:        extrapolate using segment [0, 1]
//   axis[i] <= query < axis[i+1]: interpolate within segment [i, i+1]
//   query >= axis[N-2]:      extrapolate using the last segment [N-2, N-1]
//
// The result is stored through the f32() REAL*4 boundary before return.

#include <vector>

namespace datcom_equivalent::common {

double linearInterpolate(const std::vector<double>& axis,
                          const std::vector<double>& values,
                          double query);

// General-purpose linear interpolation/extrapolation, faithfully ported
// from the separate (textually distinct) `_linear` helper defined in
// third_party/datcom_equivalent_oracle/datcom_equivalent/body/normal.py
// (NOT the same function as linearInterpolate() above -- that one mirrors
// crossflow_cdc.py's `_linear`, which never receives a descending or
// single-element axis; normal.py's BODY tensor tables do, via their
// trivial single-value "station" axis, so this variant carries the extra
// cases the CDC one deliberately does not need). Do not merge the two.
//
// Extra behavior beyond linearInterpolate():
//   - axis.size() == 1: returns f32(values[0]) directly (used by the
//     BODY sharp-cone tensor tables' trivial station axis).
//   - supports a descending axis (axis[1] < axis[0]) as well as ascending.
//   - guards against a zero-width bracketing segment (adjacent equal axis
//     values) by returning the lower value directly instead of dividing
//     by zero.
double linearInterpolateGeneral(const std::vector<double>& axis,
                                 const std::vector<double>& values,
                                 double query);

}  // namespace datcom_equivalent::common
