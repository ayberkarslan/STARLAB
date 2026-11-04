#include "aero/DragModels.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double PI = 3.14159265358979323846;
constexpr double AIR_DYNAMIC_VISCOSITY = 1.81e-5;
} // namespace

namespace DragModels {

double referenceAreaFromDiameter(double diameter) {
    if (diameter <= 0.0) {
        return 0.0;
    }
    return PI * diameter * diameter * 0.25;
}

double safeReferenceArea(const FlightCondition& fc) {
    if (fc.referenceArea > 0.0) {
        return fc.referenceArea;
    }
    return referenceAreaFromDiameter(fc.referenceDiameter);
}

double reynoldsForLength(const FlightCondition& fc, double length) {
    if (fc.reynolds > 0.0) {
        return fc.reynolds;
    }
    if (fc.rho <= 0.0 || fc.velocity <= 0.0 || length <= 0.0) {
        return 0.0;
    }
    return fc.rho * fc.velocity * length / AIR_DYNAMIC_VISCOSITY;
}

double skinFrictionCf(double mach, double reynolds) {
    if (reynolds <= 0.0) {
        return 0.0;
    }
    const double re = std::max(reynolds, 1.0);
    const double cf = 0.455 / std::pow(std::log10(re), 2.58);
    const double compressibility = std::pow(std::max(0.05, 1.0 + 0.144 * mach * mach), 0.65);
    return cf / compressibility;
}

double toGlobalCd(double cdLocal, double localArea, double referenceArea) {
    if (referenceArea <= 0.0 || localArea <= 0.0) {
        return 0.0;
    }
    return cdLocal * localArea / referenceArea;
}

double waveDrag(double mach, double strength) {
    if (mach <= 0.9) {
        return 0.0;
    }
    const double x = mach - 0.9;
    return strength * 0.02 * x * x;
}

} // namespace DragModels
