// Portions derived from MAPLEAF
// Copyright (c) 2020 Henry Stoldt, AERO-CORE, and contributors
// Licensed under the MIT License.
// Upstream source: https://github.com/henrystoldt/MAPLEAF
// Upstream commit: af970d3e8200832f5e70d537b15ad38dd74fa551

#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <limits>

namespace RoketSiM::Mapleaf {

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kGamma = 1.4;
constexpr double kGasConstant = 287.05; // J/(kg*K)

struct Vector3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vector3() = default;
    constexpr Vector3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vector3 operator/(double s) const { return {x / s, y / s, z / s}; }

    Vector3& operator+=(const Vector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vector3& operator-=(const Vector3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vector3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }

    double dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3 cross(const Vector3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    double lengthSquared() const { return x * x + y * y + z * z; }
    double length() const { return std::sqrt(lengthSquared()); }

    Vector3 normalized() const {
        double len = length();
        if (len <= 1.0e-15) return {0.0, 0.0, 0.0};
        return *this / len;
    }
};

inline Vector3 operator*(double s, const Vector3& v) { return v * s; }

struct ForceMoment {
    Vector3 force{0.0, 0.0, 0.0};
    Vector3 moment{0.0, 0.0, 0.0};
    Vector3 location{0.0, 0.0, 0.0}; // Point of application

    ForceMoment() = default;
    ForceMoment(const Vector3& f, const Vector3& loc, const Vector3& m = {0.0, 0.0, 0.0})
        : force(f), moment(m), location(loc) {}

    ForceMoment operator+(const ForceMoment& o) const {
        // Sum forces, transform moments to common reference (or sum if collocated)
        // M_total = M1 + M2 + (r1 - r_ref) x F1 + (r2 - r_ref) x F2
        // If reference is location of this:
        Vector3 extraMoment = (o.location - location).cross(o.force);
        return ForceMoment(force + o.force, location, moment + o.moment + extraMoment);
    }

    ForceMoment& operator+=(const ForceMoment& o) {
        *this = *this + o;
        return *this;
    }
};

struct FinSlice {
    double radius = 0.0;        // Distance from centerline to slice center [m]
    double area = 0.0;          // Area of slice [m^2]
    double leadingEdgeZ = 0.0;  // Z position of LE at slice [m]
    double length = 0.0;        // Chord length of slice [m]
};

struct ModelMetadata {
    std::string provider = "mapleaf_reference_only";
    std::string upstreamCommit = "af970d3e8200832f5e70d537b15ad38dd74fa551";
    std::string model = "historical_barrowman_fin_slice_reference";
    std::string regime = "subsonic";
    bool validatedAgainstUpstream = true;
    bool kstarEnabled = false;
    double bodyOnFinFactor = 1.0;
    double finNumberFactor = 1.0;
    int nativeFinCount = 4;
    std::vector<std::string> knownLimitations = {
        "Kstar_busemann_disabled",
        "transonic_interpolation_only",
        "empirical_fin_count_correction_above_4_fins",
        "stall_clipping",
        "no_viscous_cfd_interaction",
        "no_shock_boundary_layer_interaction"
    };
};

} // namespace RoketSiM::Mapleaf
