// Portions of the local-flow and optional Mach-cone execution approach are
// derived from MAPLEAF (MIT), commit af970d3e8200832f5e70d537b15ad38dd74fa551.
#include "aero/DynamicFinAero.hpp"

#include <algorithm>
#include <cmath>

namespace RoketSiM::DynamicAero {
namespace {
constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kVelocityEpsilon = 1.0e-9;

Vector3 rotateAroundUnitAxis(const Vector3& vector, const Vector3& axis, double angle) {
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return vector * c + axis.cross(vector) * s + axis * (axis.dot(vector) * (1.0 - c));
}

double signedIncidence(const Vector3& normal, const Vector3& localAir, double maximum) {
    const double speed = localAir.length();
    if (speed < kVelocityEpsilon) return 0.0;
    return std::clamp(std::asin(std::clamp(normal.dot(localAir) / speed, -1.0, 1.0)),
                      -maximum, maximum);
}

double machConeCorrection(double coneEdgeZ, const DynamicFinSlice& slice) {
    if (slice.chord <= 1.0e-12) return 1.0;
    const double outside = std::clamp(std::abs(coneEdgeZ - slice.leadingEdgeZ) / slice.chord, 0.0, 1.0);
    return outside + 0.5 * (1.0 - outside);
}

struct Load { Vector3 force; Vector3 moment; };

} // namespace

bool StaticFinCoefficients::isUsable() const noexcept {
    return coverage == StaticFinSlopeCoverage::RawSingleFin
        && normalForceSlopePerFin > 0.0 && planformArea > 0.0;
}

double DynamicFinGeometry::planformArea() const noexcept {
    return span > 0.0 ? span * (rootChord + tipChord) * 0.5 : 0.0;
}

bool DynamicFinGeometry::precompute(std::string* problem) {
    slices.clear();
    orientations.clear();
    if (finCount < 3 || span <= 0.0 || rootChord <= 0.0 || tipChord < 0.0 || bodyRadius < 0.0) {
        if (problem) *problem = "Dynamic fin geometry requires finCount >= 3, positive span/root chord, and non-negative radii.";
        return false;
    }
    sliceCount = std::max(sliceCount, 1);
    const double dz = span / static_cast<double>(sliceCount);
    const double slope = (tipChord - rootChord) / span;
    const double leSlope = std::tan(leadingEdgeSweepRad);
    slices.reserve(static_cast<std::size_t>(sliceCount));
    for (int index = 0; index < sliceCount; ++index) {
        const double y = (static_cast<double>(index) + 0.5) * dz;
        const double chord = rootChord + slope * y;
        if (chord <= 0.0) {
            if (problem) *problem = "Dynamic fin geometry has a non-positive chord within the span.";
            slices.clear();
            return false;
        }
        slices.push_back({bodyRadius + y, chord * dz, chord, y * leSlope});
    }
    orientations.reserve(static_cast<std::size_t>(finCount));
    for (int index = 0; index < finCount; ++index) {
        const double azimuth = firstFinAzimuthRad + 2.0 * kPi * index / finCount;
        const Vector3 spanDirection{std::cos(azimuth), std::sin(azimuth), 0.0};
        orientations.push_back({index, azimuth, spanDirection,
                                {spanDirection.y, -spanDirection.x, 0.0}});
    }
    return true;
}

DynamicAeroResult DynamicFinModel::evaluateDelta(const DynamicFinState& state) const {
    DynamicAeroResult result;
    if (!options.enabled) return result;
    if (!staticCoefficients.isUsable()) {
        result.valid = false;
        result.problem = "Dynamic fin model requires RAW_SINGLE_FIN static C_Nalpha and a positive one-fin planform area; vehicle/fin-set coefficients are rejected to prevent double counting.";
        return result;
    }
    if (geometry.slices.empty() || geometry.orientations.empty()) {
        result.valid = false;
        result.problem = "Dynamic fin geometry has not been precomputed.";
        return result;
    }
    if (state.dynamicPressure <= 0.0 || state.airVelocity.length() < kVelocityEpsilon) return result;
    if (std::abs(staticCoefficients.planformArea - geometry.planformArea()) > 1.0e-8 * std::max(1.0, geometry.planformArea())) {
        result.valid = false;
        result.problem = "Dynamic fin planform area differs from the canonical component-level static coefficient contract.";
        return result;
    }

    const double referenceSpeedSquared = state.airVelocity.lengthSquared();
    const bool busemann = options.experimentalSupersonicBusemann && state.mach >= FinSetBarrowman::kSupersonicLimit;
    if (busemann) result.supersonicModel = options.experimentalMachConeCorrection
        ? "experimental_busemann_with_mapleaf_mach_cone" : "experimental_busemann";
    const auto coefficients = busemann ? FinSetBarrowman::supersonicCoefficients(state.mach)
                                       : FinSetBarrowman::SupersonicCoefficients{};
    const double beta = busemann ? std::sqrt(std::max(state.mach * state.mach - 1.0, 0.0)) : 0.0;
    const double outerRadius = geometry.slices.back().radius;
    const double tipZ = geometry.slices.back().leadingEdgeZ;

    auto integrate = [&](bool dynamic) {
        Load load{};
        for (const auto& orientation : geometry.orientations) {
            const double deflection = dynamic && orientation.finIndex < static_cast<int>(state.finDeflectionsRad.size())
                ? state.finDeflectionsRad[orientation.finIndex] : 0.0;
            const Vector3 normal = rotateAroundUnitAxis(orientation.undeflectedNormal, orientation.spanDirection, deflection).normalized();
            for (const auto& slice : geometry.slices) {
                const Vector3 position = orientation.spanDirection * slice.radius
                    + Vector3{0.0, 0.0, geometry.rootLeadingEdgeZ + slice.leadingEdgeZ};
                const Vector3 localAir = dynamic && options.localFinFlow
                    ? state.airVelocity - state.angularVelocity.cross(position - state.cgPosition)
                    : state.airVelocity;
                const double incidence = signedIncidence(normal, localAir, options.maximumLocalIncidenceRad);
                const double qLocal = state.dynamicPressure * localAir.lengthSquared() / referenceSpeedSquared;
                double normalCoefficient = staticCoefficients.normalForceSlopePerFin * incidence;
                if (busemann) {
                    // Existing RoketSiM Busemann coefficients, not a MAPLEAF duplicate.
                    normalCoefficient = 2.0 * (coefficients.k1 * incidence
                        + coefficients.k3 * incidence * incidence * incidence);
                    if (options.experimentalMachConeCorrection) {
                        const double coneEdge = (outerRadius - slice.radius) * beta + tipZ;
                        normalCoefficient *= machConeCorrection(coneEdge, slice);
                    }
                }
                const Vector3 force = normal * (qLocal * slice.area * normalCoefficient);
                load.force += force;
                load.moment += (position - state.cgPosition).cross(force);
            }
        }
        return load;
    };

    const Load staticLoad = integrate(false);
    const Load dynamicLoad = integrate(true);
    result.fullFinForce = dynamicLoad.force;
    result.fullFinMoment = dynamicLoad.moment;
    result.deltaForce = dynamicLoad.force - staticLoad.force;
    result.deltaMoment = dynamicLoad.moment - staticLoad.moment;
    return result;
}

DynamicAeroResult DistributedBodyDampingModel::evaluateDelta(const DynamicFinState& state) const {
    DynamicAeroResult result;
    if (!enabled) return result;
    if (radius <= 0.0 || length <= 0.0 || axialSegments < 1 || circumferentialSegments < 3
        || state.dynamicPressure <= 0.0 || state.airVelocity.length() < kVelocityEpsilon) {
        result.valid = false;
        result.problem = "Distributed body damping requires valid cylinder geometry, segmentation, dynamic pressure, and airspeed.";
        return result;
    }
    const double dz = length / axialSegments;
    const double dphi = 2.0 * kPi / circumferentialSegments;
    const double speedSquared = state.airVelocity.lengthSquared();
    for (int zi = 0; zi < axialSegments; ++zi) {
        const double z = startZ + (zi + 0.5) * dz;
        for (int pi = 0; pi < circumferentialSegments; ++pi) {
            const double phi = (pi + 0.5) * dphi;
            const Vector3 normal{std::cos(phi), std::sin(phi), 0.0};
            const Vector3 position = normal * radius + Vector3{0.0, 0.0, z};
            const Vector3 localAir = state.airVelocity - state.angularVelocity.cross(position - state.cgPosition);
            const double incidence = signedIncidence(normal, localAir, kPi / 2.0);
            const double qLocal = state.dynamicPressure * localAir.lengthSquared() / speedSquared;
            const double panelArea = radius * dphi * dz;
            const Vector3 force = normal * (qLocal * panelArea * empiricalNormalCoefficient * incidence);
            result.deltaForce += force;
            result.deltaMoment += (position - state.cgPosition).cross(force);
        }
    }
    result.fullFinForce = result.deltaForce;
    result.fullFinMoment = result.deltaMoment;
    return result;
}

} // namespace RoketSiM::DynamicAero
