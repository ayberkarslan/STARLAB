// Dynamic fin-loading extension.  Static coefficients remain owned by the
// canonical RoketSiM Barrowman/OpenRocket-like model.
#pragma once

#include "aero/FinSetBarrowman.hpp"
#include <cmath>
#include <string>
#include <vector>

namespace RoketSiM::DynamicAero {

struct Vector3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    constexpr Vector3() = default;
    constexpr Vector3(double xValue, double yValue, double zValue)
        : x(xValue), y(yValue), z(zValue) {}
    Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vector3 operator*(double scale) const { return {x * scale, y * scale, z * scale}; }
    Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    double dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }
    Vector3 cross(const Vector3& other) const { return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x}; }
    double lengthSquared() const { return dot(*this); }
    double length() const { return std::sqrt(lengthSquared()); }
    Vector3 normalized() const { const double value = length(); return value > 1.0e-15 ? *this * (1.0 / value) : Vector3{}; }
};

// This contract deliberately excludes vehicle-level coefficients: distributing
// one across fins would hide interference and reference-area double counting.
enum class StaticFinSlopeCoverage {
    RawSingleFin,
    FinSetAfterInternalInterference,
    FullVehicle,
    Unspecified,
};

struct StaticFinCoefficients {
    // C_Nalpha of one physical fin, normalized by that fin's planform area
    // [1/rad].  It must not contain body-on-fin or fin-number interference.
    double normalForceSlopePerFin = 0.0;
    double planformArea = 0.0; // m^2; must equal the represented fin planform.
    StaticFinSlopeCoverage coverage = StaticFinSlopeCoverage::Unspecified;
    std::string source = "";

    bool isUsable() const noexcept;
};

struct DynamicFinSlice {
    double radius = 0.0;       // m from longitudinal centreline
    double area = 0.0;         // m^2, one-fin planform slice
    double chord = 0.0;        // m
    double leadingEdgeZ = 0.0; // m from the fin root leading edge
};

struct DynamicFinOrientation {
    int finIndex = 0;
    double azimuthRad = 0.0;
    Vector3 spanDirection{1.0, 0.0, 0.0};
    Vector3 undeflectedNormal{0.0, -1.0, 0.0};
};

// Geometry only.  It has no MAC, CP, interference, or static-aero formula;
// those remain the canonical model's responsibility.
struct DynamicFinGeometry {
    int finCount = 0;
    double firstFinAzimuthRad = 0.0;
    double rootLeadingEdgeZ = 0.0; // nose datum, m
    double bodyRadius = 0.0;       // m
    double span = 0.0;             // m
    double rootChord = 0.0;        // m
    double tipChord = 0.0;         // m
    double leadingEdgeSweepRad = 0.0;
    int sliceCount = 40;
    std::vector<DynamicFinSlice> slices;
    std::vector<DynamicFinOrientation> orientations;

    bool precompute(std::string* problem = nullptr);
    double planformArea() const noexcept;
};

struct DynamicFinState {
    // RoketSiM body frame: +Z is longitudinal/forward; +X/+Y are transverse.
    // airVelocity is V_air - V_CG in that frame. angularVelocity is [rad/s].
    Vector3 airVelocity{0.0, 0.0, 0.0};
    Vector3 angularVelocity{0.0, 0.0, 0.0};
    Vector3 cgPosition{0.0, 0.0, 0.0}; // nose datum, m
    double dynamicPressure = 0.0;      // Pa at |airVelocity|
    double mach = 0.0;
    std::vector<double> finDeflectionsRad;
};

struct DynamicFinOptions {
    bool enabled = false;                 // production default
    bool localFinFlow = true;
    bool spanwiseIntegration = true;
    bool experimentalSupersonicBusemann = false;
    bool experimentalMachConeCorrection = false; // OFF by default
    double maximumLocalIncidenceRad = 0.6108652381980153; // 35 degrees
};

struct DynamicAeroResult {
    Vector3 deltaForce{0.0, 0.0, 0.0};   // Add to canonical static result [N]
    Vector3 deltaMoment{0.0, 0.0, 0.0};  // about CG [N m]
    Vector3 fullFinForce{0.0, 0.0, 0.0}; // diagnostics only, not additive
    Vector3 fullFinMoment{0.0, 0.0, 0.0};
    bool valid = true;
    std::string problem;
    std::string supersonicModel = "canonical_static_delta";
};

class DynamicFinModel {
public:
    DynamicFinGeometry geometry;
    StaticFinCoefficients staticCoefficients;
    DynamicFinOptions options;

    DynamicAeroResult evaluateDelta(const DynamicFinState& state) const;
};

// Experimental distributed cylinder crossflow damping.  It is intentionally a
// delta model and cannot change static production aerodynamics when disabled.
struct DistributedBodyDampingModel {
    bool enabled = false;
    double radius = 0.0;
    double startZ = 0.0;
    double length = 0.0;
    int axialSegments = 40;
    int circumferentialSegments = 24;
    double empiricalNormalCoefficient = 1.0;

    DynamicAeroResult evaluateDelta(const DynamicFinState& state) const;
};

} // namespace RoketSiM::DynamicAero
