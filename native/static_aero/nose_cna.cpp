#include "../export.hpp"

STARLAB_EXPORT double static_nose_cna(
    double, double radius, double, double referenceRadius,
    double overrideEnabled, double overrideValue, double, double
) {
    if (overrideEnabled != 0.0) return overrideValue;
    if (referenceRadius <= 0.0) return 0.0;
    return 2.0 * radius * radius / (referenceRadius * referenceRadius);
}
