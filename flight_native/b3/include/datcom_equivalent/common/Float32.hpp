#pragma once

// REAL*4 store-truncation helper.
//
// Faithful port of third_party/datcom_equivalent_oracle/
// datcom_equivalent/common/float32.py::real4(). Every REAL*4 store boundary in
// the ported physics depends on this being bit-exact.
//
// f32() takes a double (matching every intermediate value in the oracle,
// since Python floats are always float64) and returns a double that is
// exactly representable as an IEEE-754 binary32 value -- i.e. the same
// double-rounding-free semantics as
// struct.unpack("<f", struct.pack("<f", value))[0].
//
// Deliberately returns `double`, not `float`: if this returned `float`,
// C++ would implicitly promote back to double at every use site anyway,
// but *intermediate* float+float arithmetic done directly in `float`
// precision before the next explicit f32() call would round twice
// (once per float op, then again at the next f32() store) instead of
// once -- diverging from the oracle, which always computes in float64
// between explicit real4() truncations. Do not change this to `float`.

#include <cstdint>
#include <cstring>

namespace datcom_equivalent::common {

inline double f32(double value) noexcept {
    float truncated = static_cast<float>(value);
    return static_cast<double>(truncated);
}

// Bit pattern of the IEEE-754 binary32 representation of an already
// f32()-truncated value (little-endian byte order is irrelevant here --
// this returns the logical bit pattern, not a memory layout).
// Mirrors crossflow_cdc.py::fbits().
inline std::uint32_t f32Bits(double value) noexcept {
    float truncated = static_cast<float>(value);
    std::uint32_t bits = 0;
    std::memcpy(&bits, &truncated, sizeof(bits));
    return bits;
}

}  // namespace datcom_equivalent::common
