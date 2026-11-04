#include "datcom_equivalent/common/Interpolation.hpp"

#include <algorithm>
#include <cstddef>

#include "datcom_equivalent/common/Float32.hpp"

namespace datcom_equivalent::common {

double linearInterpolate(const std::vector<double>& axis,
                          const std::vector<double>& values,
                          double query) {
    // 1:1 translation of the Python _linear's 1-based `ordinal`/`upper`
    // scan, preserved exactly (including the clamp) rather than rewritten
    // against 0-based idioms, so the predicate order matches the source.
    const int n = static_cast<int>(axis.size());
    int upper = n;
    for (int ordinal = 2; ordinal <= n; ++ordinal) {
        upper = ordinal;
        if (query < axis[static_cast<std::size_t>(upper - 1)]) {
            break;
        }
    }
    upper = std::min(std::max(upper, 2), n);
    const int lower = upper - 2;

    const double lowerAxis = axis[static_cast<std::size_t>(lower)];
    const double upperAxis = axis[static_cast<std::size_t>(upper - 1)];
    const double lowerValue = values[static_cast<std::size_t>(lower)];
    const double upperValue = values[static_cast<std::size_t>(upper - 1)];

    return f32(lowerValue + (query - lowerAxis) * (upperValue - lowerValue) /
                                (upperAxis - lowerAxis));
}

double linearInterpolateGeneral(const std::vector<double>& axis,
                                 const std::vector<double>& values,
                                 double query) {
    const int n = static_cast<int>(axis.size());
    if (n == 1) {
        return f32(values[0]);
    }
    const bool descending = axis[1] < axis[0];
    int upper = n;
    for (int ordinal = 2; ordinal <= n; ++ordinal) {
        upper = descending ? (n - ordinal + 2) : ordinal;
        if (query < axis[static_cast<std::size_t>(upper - 1)]) {
            break;
        }
    }
    if (descending) {
        upper += 1;
    }
    upper = std::min(std::max(upper, 2), n);
    const int low = upper - 2;

    const double axisLow = axis[static_cast<std::size_t>(low)];
    const double axisNext = axis[static_cast<std::size_t>(low + 1)];
    const double valueLow = values[static_cast<std::size_t>(low)];
    const double valueNext = values[static_cast<std::size_t>(low + 1)];

    if (axisNext == axisLow) {
        return f32(valueLow);
    }
    return f32(valueLow + (query - axisLow) * (valueNext - valueLow) / (axisNext - axisLow));
}

}  // namespace datcom_equivalent::common
