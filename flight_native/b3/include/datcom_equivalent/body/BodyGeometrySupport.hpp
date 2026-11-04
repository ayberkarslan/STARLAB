#pragma once

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "datcom_equivalent/Types.hpp"
#include "datcom_equivalent/common/Float32.hpp"
#include "datcom_equivalent/common/Interpolation.hpp"

namespace datcom_equivalent::body {

// Source-equivalent sharp tangent-ogive geometry recovered from OGIVE,
// DSPLAN, DSWET, DVOL, DXCENV and DXCENP. Values are stored at each
// material default-REAL boundary so callers can use either SI or DATCOM's
// internal length unit without changing the operation ordering.
struct BodyProfileProperties {
    std::vector<double> contourX;
    std::vector<double> contourR;
    double planform = 0.0;
    double wetted = 0.0;
    double volume = 0.0;
    double noseVolume = 0.0;
    double volumeCentroid = 0.0;
    double planformCentroid = 0.0;
    double theoreticalNoseLength = 0.0;
    double theoreticalNoseFineness = 0.0;
};

namespace geometry_detail {

using datcom_equivalent::common::f32;
constexpr double kPiReal4 = 3.1415927410125732421875;

struct SegmentMetrics {
    double planform = 0.0;
    double wetted = 0.0;
    double volume = 0.0;
    double volumeCentroid = 0.0;
    double planformCentroid = 0.0;
};

inline SegmentMetrics integrate(const std::array<double, 101>& x,
                                const std::array<double, 101>& r) {
    SegmentMetrics out;
    double volumeMoment = 0.0;
    double planformMoment = 0.0;
    for (std::size_t i = 1; i < x.size(); ++i) {
        const double dx = f32(x[i] - x[i - 1]);
        const double dr = f32(r[i] - r[i - 1]);
        const double radiusSum = f32(r[i] + r[i - 1]);
        const double deltaPlanform = f32(dx * radiusSum);
        out.planform = f32(out.planform + deltaPlanform);
        out.wetted = f32(out.wetted + f32(kPiReal4 * radiusSum *
            std::sqrt(f32(dx * dx + dr * dr))));

        const double r0Squared = f32(r[i - 1] * r[i - 1]);
        const double r1Squared = f32(r[i] * r[i]);
        const double radiusProduct = f32(r[i] * r[i - 1]);
        const double volumeDenominator = f32(r0Squared + r1Squared + radiusProduct);
        const double deltaVolume = f32(f32(kPiReal4 / f32(3.0)) * dx * volumeDenominator);
        out.volume = f32(out.volume + deltaVolume);

        double planformCenter = 0.0;
        double volumeCenter = 0.0;
        if (r[i] >= r[i - 1]) {
            planformCenter = f32(x[i] - f32(f32(dx / f32(3.0)) *
                f32(f32(f32(2.0) * r[i - 1]) + r[i]) / radiusSum));
            volumeCenter = f32(x[i] - f32(f32(dx / f32(4.0)) *
                f32(r1Squared + f32(f32(2.0) * radiusProduct) +
                    f32(f32(3.0) * r0Squared)) / volumeDenominator));
        } else {
            planformCenter = f32(x[i - 1] + f32(f32(dx / f32(3.0)) *
                f32(f32(f32(2.0) * r[i]) + r[i - 1]) / radiusSum));
            volumeCenter = f32(x[i - 1] + f32(f32(dx / f32(4.0)) *
                f32(r0Squared + f32(f32(2.0) * radiusProduct) +
                    f32(f32(3.0) * r1Squared)) / volumeDenominator));
        }
        planformMoment = f32(planformMoment + f32(planformCenter * deltaPlanform));
        volumeMoment = f32(volumeMoment + f32(volumeCenter * deltaVolume));
    }
    out.planformCentroid = f32(planformMoment / out.planform);
    out.volumeCentroid = f32(volumeMoment / out.volume);
    return out;
}

inline void tangentOgiveDense(double length, double diameter,
                              std::array<double, 101>& x,
                              std::array<double, 101>& r,
                              double& theoreticalLength,
                              double& theoreticalFineness) {
    const double half = f32(diameter / f32(2.0));
    const double eta = f32(f32(f32(length * length) + f32(half * half)) / diameter);
    theoreticalLength = f32(diameter * std::sqrt(f32(f32(eta / diameter) - f32(0.25))));
    theoreticalFineness = f32(theoreticalLength / diameter);
    const double offset = f32(theoreticalLength - length);
    const double step = f32(length / f32(100.0));
    const double fSquared = f32(theoreticalFineness * theoreticalFineness);
    const double core = f32(fSquared - f32(0.25));
    const double constant = f32(fSquared + f32(0.25));
    for (std::size_t i = 0; i < x.size(); ++i) {
        x[i] = i == 100 ? f32(offset + length)
                        : f32(offset + f32(step * static_cast<double>(i)));
        const double normalized = f32(x[i] / theoreticalLength);
        const double radial = f32(f32(core * core) +
            f32(fSquared * f32(f32(f32(2.0) * normalized) - f32(normalized * normalized))));
        r[i] = f32(half * f32(f32(1.0) + f32(f32(2.0) *
            f32(std::sqrt(radial) - constant))));
        if (i == 100) r[i] = half;
    }
}

inline SegmentMetrics cylinderDense(double origin, double length, double radius,
                                    std::array<double, 101>& x,
                                    std::array<double, 101>& r) {
    const double step = f32(length / f32(100.0));
    for (std::size_t i = 0; i < x.size(); ++i) {
        x[i] = i == 100 ? f32(origin + length)
                        : f32(origin + f32(step * static_cast<double>(i)));
        r[i] = radius;
    }
    return integrate(x, r);
}

}  // namespace geometry_detail

inline BodyProfileProperties tangentOgiveCylinderProperties(double noseLength,
                                                             double diameter,
                                                             double centerLength) {
    using namespace geometry_detail;
    if (noseLength <= 0.0 || diameter <= 0.0 || centerLength < 0.0) {
        throw std::invalid_argument("invalid tangent-ogive/cylinder geometry");
    }
    std::array<double, 101> noseX{}, noseR{}, centerX{}, centerR{};
    BodyProfileProperties out;
    tangentOgiveDense(f32(noseLength), f32(diameter), noseX, noseR,
                      out.theoreticalNoseLength, out.theoreticalNoseFineness);
    const auto nose = integrate(noseX, noseR);
    const auto center = cylinderDense(f32(noseLength), f32(centerLength),
                                      f32(f32(diameter) / f32(2.0)), centerX, centerR);
    out.planform = f32(nose.planform + center.planform);
    out.wetted = f32(nose.wetted + center.wetted);
    out.volume = f32(nose.volume + center.volume);
    out.noseVolume = nose.volume;
    out.planformCentroid = f32(f32(f32(nose.planformCentroid * nose.planform) +
        f32(center.planformCentroid * center.planform)) / out.planform);
    out.volumeCentroid = f32(f32(f32(nose.volumeCentroid * nose.volume) +
        f32(center.volumeCentroid * center.volume)) / out.volume);
    out.contourX.reserve(21);
    out.contourR.reserve(21);
    for (std::size_t i = 0; i <= 100; i += 10) {
        out.contourX.push_back(noseX[i]);
        out.contourR.push_back(noseR[i]);
    }
    for (std::size_t i = 10; i <= 100; i += 10) {
        out.contourX.push_back(centerX[i]);
        out.contourR.push_back(centerR[i]);
    }
    return out;
}

inline double bodyRadiusAtStation(const BodyGeometry& body, double x) {
    using datcom_equivalent::common::f32;
    if (body.noseLengthM <= 0.0 || body.diameterM <= 0.0) {
        throw std::invalid_argument("invalid BODY geometry");
    }
    const double localX = f32(f32(x) - f32(body.noseOriginM));
    if (body.noseFamily == BodyNoseFamily::SharpCone) {
        if (localX <= f32(body.noseLengthM)) {
            return f32(f32(f32(0.5) * f32(body.diameterM)) * localX /
                       f32(body.noseLengthM));
        }
        return f32(f32(0.5) * f32(body.diameterM));
    }
    const auto profile = tangentOgiveCylinderProperties(
        f32(body.noseLengthM), f32(body.diameterM), f32(body.centerLengthM));
    return common::linearInterpolateGeneral(profile.contourX, profile.contourR, localX);
}

}  // namespace datcom_equivalent::body
