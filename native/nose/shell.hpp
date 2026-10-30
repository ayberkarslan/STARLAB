#pragma once
#include "profile.hpp"
// RoketSiM src/cli/design_analysis.hpp::integrateShell, unchanged numerical body.
namespace starlab_nose {
struct Properties {double length=0,volume=0,mass=0,localCg=0,iAxial=0,iTransverse=0;};
template <typename RadiusFunction>
inline void integrateShell(
    double length,
    double thickness,
    double density,
    RadiusFunction radius,
    Properties& result
) {
    // Match OpenRocket SymmetricComponent.calculateProperties(): it divides
    // the profile into 128 conical frusta and converts a wall-normal thickness
    // into each frustum's radial inner profile.  A radial-only shell is
    // noticeably light for an ogive nose cone.
    constexpr int slices = 128;
    if (length <= 0.0) return;
    const double dz = length / slices;
    double firstMoment = 0.0;
    double transverseOrigin = 0.0;
    for (int i = 0; i < slices; ++i) {
        const double z0 = i * dz;
        const double z1 = (i + 1) * dz;
        const double ro0 = std::max(radius(z0), 0.0);
        const double ro1 = std::max(radius(z1), 0.0);
        const double radialWall = std::max(thickness, 0.0)
            * std::hypot(ro1 - ro0, dz) / dz;
        const double ri0 = std::max(ro0 - radialWall, 0.0);
        const double ri1 = std::max(ro1 - radialWall, 0.0);
        const auto frustumVolume = [&](double r0, double r1) {
            return kPi * dz * (r0 * r0 + r0 * r1 + r1 * r1) / 3.0;
        };
        const auto frustumCg = [&](double r0, double r1) {
            const double denominator = r0 * r0 + r0 * r1 + r1 * r1;
            return denominator > 1.0e-18 ? dz * (r0 * r0 + 2.0 * r0 * r1
                + 3.0 * r1 * r1) / (4.0 * denominator) : dz / 2.0;
        };
        const double outerVolume = frustumVolume(ro0, ro1);
        const double innerVolume = frustumVolume(ri0, ri1);
        const double dVolume = outerVolume - innerVolume;
        const double dm = density * dVolume;
        const double localCg = dVolume > 1.0e-18
            ? (outerVolume * frustumCg(ro0, ro1)
                - innerVolume * frustumCg(ri0, ri1)) / dVolume : dz / 2.0;
        const double z = z0 + localCg;
        result.volume += dVolume;
        result.mass += dm;
        firstMoment += dm * z;
        const double ro = (ro0 + ro1) / 2.0;
        const double ri = (ri0 + ri1) / 2.0;
        result.iAxial += 0.5 * dm * (ro * ro + ri * ri);
        transverseOrigin +=
            dm * (0.25 * (ro * ro + ri * ri) + z * z);
    }
    if (result.mass > 0.0) {
        result.localCg = firstMoment / result.mass;
        result.iTransverse =
            std::max(transverseOrigin - result.mass * result.localCg
                * result.localCg, 0.0);
    }
}
}
