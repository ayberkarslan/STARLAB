#pragma once

#include <algorithm>
#include <cmath>

namespace FinSetBarrowman {

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kGamma = 1.4;
constexpr double kStallAngle = 20.0 * kPi / 180.0;
constexpr double kSubsonicLimit = 0.9;
constexpr double kSupersonicLimit = 1.5;
constexpr double kCoefficientTableMaxMach = 4.9;

struct SupersonicCoefficients {
    double k1 = 0.0;
    double k2 = 0.0;
    double k3 = 0.0;
};

inline double limitedAngleOfAttack(double angleOfAttack) {
    double alpha = std::fmod(std::abs(angleOfAttack), 2.0 * kPi);
    if (alpha > kPi) alpha = 2.0 * kPi - alpha;
    return std::min({alpha, kPi - alpha, kStallAngle});
}

inline SupersonicCoefficients exactSupersonicCoefficients(double mach) {
    const double beta = std::sqrt(std::max(mach * mach - 1.0, 1.0e-14));
    const double beta2 = beta * beta;
    const double mach2 = mach * mach;
    const double mach4 = mach2 * mach2;
    const double mach6 = mach4 * mach2;
    const double mach8 = mach4 * mach4;
    return {
        2.0 / beta,
        ((kGamma + 1.0) * mach4 - 4.0 * beta2) / (4.0 * beta2 * beta2),
        ((kGamma + 1.0) * mach8
            + (2.0 * kGamma * kGamma - 7.0 * kGamma - 5.0) * mach6
            + 10.0 * (kGamma + 1.0) * mach4 + 8.0)
            / (6.0 * std::pow(beta, 7.0)),
    };
}

// FinSetCalc samples K1/K2/K3 every Mach 0.1 from 1.5 through 4.9.
inline SupersonicCoefficients supersonicCoefficients(double mach) {
    const double bounded = std::clamp(
        mach, kSupersonicLimit, kCoefficientTableMaxMach);
    const double position = (bounded - kSupersonicLimit) * 10.0;
    const int lowerIndex = std::clamp(
        static_cast<int>(std::floor(position + 1.0e-12)), 0, 34);
    const int upperIndex = std::min(lowerIndex + 1, 34);
    const double lowerMach = kSupersonicLimit + 0.1 * lowerIndex;
    const double upperMach = kSupersonicLimit + 0.1 * upperIndex;
    const auto lower = exactSupersonicCoefficients(lowerMach);
    if (upperIndex == lowerIndex) return lower;
    const auto upper = exactSupersonicCoefficients(upperMach);
    const double fraction = std::clamp(
        (bounded - lowerMach) / (upperMach - lowerMach), 0.0, 1.0);
    return {
        lower.k1 + fraction * (upper.k1 - lower.k1),
        lower.k2 + fraction * (upper.k2 - lower.k2),
        lower.k3 + fraction * (upper.k3 - lower.k3),
    };
}

// Quartic constrained by value and first derivative at both endpoints plus
// zero second derivative at the lower endpoint, matching PolyInterpolator.
inline double quarticEndpointInterpolation(
    double x, double x0, double x1,
    double value0, double value1,
    double derivative0, double derivative1
) {
    const double interval = x1 - x0;
    const double t = std::clamp((x - x0) / interval, 0.0, 1.0);
    const double a0 = value0;
    const double a1 = interval * derivative0;
    const double difference = value1 - a0 - a1;
    const double derivativeDifference = interval * derivative1 - a1;
    const double a3 = 4.0 * difference - derivativeDifference;
    const double a4 = derivativeDifference - 3.0 * difference;
    return a0 + t * (a1 + t * t * (a3 + t * a4));
}

inline double oneFinCna(
    double area, double span, double midChordSweepCosine,
    double mach, double angleOfAttack, double referenceArea
) {
    if (area <= 1.0e-14 || span <= 1.0e-14
        || midChordSweepCosine <= 1.0e-14 || referenceArea <= 1.0e-14) {
        return 0.0;
    }
    const double alpha = limitedAngleOfAttack(angleOfAttack);
    const double areaCosine = area * midChordSweepCosine;
    auto subsonic = [&](double localMach) {
        const double ratio = span * span / areaCosine;
        const double root = std::sqrt(std::max(
            1.0 + (1.0 - localMach * localMach) * ratio * ratio, 0.0));
        return 2.0 * kPi * span * span
            / (referenceArea * (1.0 + root));
    };
    auto supersonic = [&](double localMach) {
        const auto coefficients = supersonicCoefficients(localMach);
        return area / referenceArea
            * (coefficients.k1 + coefficients.k2 * alpha
                + coefficients.k3 * alpha * alpha);
    };
    if (mach <= kSubsonicLimit) return subsonic(mach);
    if (mach >= kSupersonicLimit) return supersonic(mach);

    const double ratio = span * span / areaCosine;
    const double root = std::sqrt(std::max(
        1.0 + (1.0 - kSubsonicLimit * kSubsonicLimit)
            * ratio * ratio, 0.0));
    const double subValue = subsonic(kSubsonicLimit);
    // FinSetCalc intentionally uses the requested transonic Mach here.
    const double subDerivative =
        2.0 * mach * kPi * std::pow(span, 6.0)
        / (areaCosine * areaCosine * referenceArea
            * root * (1.0 + root) * (1.0 + root));
    const double superValue = supersonic(kSupersonicLimit);
    const double superB = std::pow(
        kSupersonicLimit * kSupersonicLimit - 1.0, 1.5);
    const double superDerivative =
        -area / referenceArea * 2.0 * kSupersonicLimit / superB;
    return quarticEndpointInterpolation(
        mach, kSubsonicLimit, kSupersonicLimit,
        subValue, superValue, subDerivative, superDerivative);
}

inline double finInterferenceFactor(int count) {
    if (count == 5) return 0.948;
    if (count == 6) return 0.913;
    if (count == 7) return 0.854;
    if (count == 8) return 0.81;
    return count > 8 ? 0.75 : 1.0;
}

inline double orientationSum(
    int count, double flowAzimuth = kPi / 2.0,
    double firstFinAngle = 0.0
) {
    if (count <= 0) return 0.0;
    double sum = 0.0;
    for (int index = 0; index < count; ++index) {
        const double finAngle = firstFinAngle + 2.0 * kPi * index / count;
        const double sine = std::sin(flowAzimuth - finAngle);
        sum += sine * sine;
    }
    return sum;
}

inline double aggregateCna(
    double area, double span, double midChordSweepCosine,
    double mach, double angleOfAttack, double referenceArea,
    int finCount, double bodyRadius, int interferenceFinCount = 0
) {
    const double isolated = oneFinCna(
        area, span, midChordSweepCosine, mach,
        angleOfAttack, referenceArea);
    const double tau = bodyRadius > 0.0
        ? bodyRadius / std::max(bodyRadius + span, 1.0e-14) : 0.0;
    return isolated * orientationSum(finCount)
        * finInterferenceFactor(interferenceFinCount > 0
            ? interferenceFinCount : finCount) * (1.0 + tau);
}

inline double cpFraction(double aspectRatio, double mach) {
    if (mach <= 0.5) return 0.25;
    auto supersonic = [&](double localMach) {
        const double beta = std::sqrt(std::max(localMach * localMach - 1.0, 0.0));
        double denominator = 2.0 * aspectRatio * beta - 1.0;
        if (std::abs(denominator) < 1.0e-12) {
            denominator = std::copysign(1.0e-12, denominator);
        }
        return (aspectRatio * beta - 0.67) / denominator;
    };
    if (mach >= 2.0) return supersonic(mach);

    const double denominator = std::pow(1.0 - 3.4641 * aspectRatio, 2.0);
    if (denominator <= 1.0e-20) return 0.25;
    const double ar = aspectRatio;
    const double coefficients[6] = {
        9.16049 * (-0.588838 + ar) * (-0.20624 + ar) / denominator,
        -31.6049 * (-0.705375 + ar) * (-0.198476 + ar) / denominator,
        55.3086 * (-0.711482 + ar) * (-0.196772 + ar) / denominator,
        -39.5062 * (-0.72074 + ar) * (-0.194245 + ar) / denominator,
        12.8395 * (-0.725688 + ar) * (-0.19292 + ar) / denominator,
        -1.58025 * (-0.728769 + ar) * (-0.192105 + ar) / denominator,
    };
    double value = 0.0;
    double power = 1.0;
    for (double coefficient : coefficients) {
        value += coefficient * power;
        power *= mach;
    }
    return value;
}

inline double normalForceCoefficient(double cna, double angleOfAttack) {
    return cna * limitedAngleOfAttack(angleOfAttack);
}

}  // namespace FinSetBarrowman
