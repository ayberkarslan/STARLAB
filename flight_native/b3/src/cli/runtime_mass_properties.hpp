// runtime_mass_properties.hpp -- the ONE canonical runtime rigid-body mass-
// property calculation the flight loop (and, later, a 6-DoF solver) reads
// from. See docs/flight/runtime_mass_properties_contract.md for the full
// coordinate-frame/units contract this implements.
//
// Deliberately plain doubles, no Eigen -- roketsim_cli.cpp's own top-of-file
// comment ("No Eigen in main loop callbacks") is a real, pre-existing
// architectural boundary between the static design-time analyzer
// (design_analysis.hpp, which does use Eigen) and the runtime flight loop.
// This header stays on that side of the boundary: every quantity here is a
// full, general (no axisymmetry assumed) but Eigen-free 3D vector / 3x3
// symmetric tensor, represented as six independent doubles.
#pragma once

#include <cmath>

namespace runtime_mass {

// A single physical mass contribution (one motor's dry structure, one
// motor's remaining propellant, or one static-structure snapshot), already
// resolved to an ABSOLUTE position in the vehicle body frame and a local
// inertia tensor about ITS OWN center of mass, in the SAME body-frame axes
// (see the coordinate-frame contract doc -- every currently-supported
// component/motor shape is a body of revolution mounted with its own axis
// parallel to the vehicle's x-axis, so its own local tensor is already
// expressed in body axes with no rotation needed; ixy/ixz/iyz are 0 for all
// of them today, but the type does not assume that).
struct MassContribution {
    double mass_kg = 0.0;
    double x_m = 0.0, y_m = 0.0, z_m = 0.0;
    double ixx_kgm2 = 0.0, iyy_kgm2 = 0.0, izz_kgm2 = 0.0;
    double ixy_kgm2 = 0.0, ixz_kgm2 = 0.0, iyz_kgm2 = 0.0;
};

// The canonical runtime output: one rigid body's mass, CG (in the body
// frame, relative to the vehicle reference origin -- see contract doc), and
// full inertia tensor ABOUT THAT CG, in body-frame axes. No axisymmetry is
// assumed; ixy_kgm2/ixz_kgm2/iyz_kgm2 are populated whenever the underlying
// contributions are actually off-axis, not zeroed by convention.
struct RuntimeMassProperties {
    double mass_kg = 0.0;
    double cg_x_m = 0.0, cg_y_m = 0.0, cg_z_m = 0.0;
    double ixx_kgm2 = 0.0, iyy_kgm2 = 0.0, izz_kgm2 = 0.0;
    double ixy_kgm2 = 0.0, ixz_kgm2 = 0.0, iyz_kgm2 = 0.0;
};

// Accumulates mass contributions about a FIXED common origin (the vehicle
// reference origin -- never about each contribution's own CG). This is the
// same "accumulate about one shared point, shift to the real CG once at the
// end" technique the pre-refactor scalar base_i_origin/getIxx() already
// used for Ixx alone; this generalizes it to the full 3D first moment and
// the full symmetric tensor (6 independent terms) so the result supports an
// off-axis CG and nonzero products of inertia when the geometry actually
// has them.
struct OriginAccumulator {
    double mass_kg = 0.0;
    double mx = 0.0, my = 0.0, mz = 0.0;  // first moment (mass * position)
    double ixx = 0.0, iyy = 0.0, izz = 0.0;
    double ixy = 0.0, ixz = 0.0, iyz = 0.0;

    // Parallel-axis shift of one contribution's own-CG tensor to the shared
    // origin: I_origin = I_cg + m*(||r||^2 * Identity - r r^T), applied
    // term-by-term (r = the contribution's own absolute position vector,
    // i.e. its displacement FROM the origin).
    void add(const MassContribution& c) {
        mass_kg += c.mass_kg;
        mx += c.mass_kg * c.x_m;
        my += c.mass_kg * c.y_m;
        mz += c.mass_kg * c.z_m;
        ixx += c.ixx_kgm2 + c.mass_kg * (c.y_m * c.y_m + c.z_m * c.z_m);
        iyy += c.iyy_kgm2 + c.mass_kg * (c.x_m * c.x_m + c.z_m * c.z_m);
        izz += c.izz_kgm2 + c.mass_kg * (c.x_m * c.x_m + c.y_m * c.y_m);
        ixy += c.ixy_kgm2 - c.mass_kg * c.x_m * c.y_m;
        ixz += c.ixz_kgm2 - c.mass_kg * c.x_m * c.z_m;
        iyz += c.iyz_kgm2 - c.mass_kg * c.y_m * c.z_m;
    }

    // Merge another origin-referenced accumulation (e.g. a precomputed
    // static-structure snapshot) into this one. Valid only because both
    // sides are referenced to the SAME shared origin -- never merge two
    // accumulators shifted to different points.
    void merge(const OriginAccumulator& other) {
        mass_kg += other.mass_kg;
        mx += other.mx; my += other.my; mz += other.mz;
        ixx += other.ixx; iyy += other.iyy; izz += other.izz;
        ixy += other.ixy; ixz += other.ixz; iyz += other.iyz;
    }

    // Removes one contribution's origin-referenced share (e.g. "the whole
    // static structure design::analyze() reported already includes the
    // motors -- take their contribution back out so the remaining baseline
    // is airframe-only"). Valid because every term in add() is linear in
    // the contribution's own mass/local-tensor values, so negating them
    // exactly undoes the add.
    void subtract(const MassContribution& c) {
        add(MassContribution{
            -c.mass_kg, c.x_m, c.y_m, c.z_m,
            -c.ixx_kgm2, -c.iyy_kgm2, -c.izz_kgm2,
            -c.ixy_kgm2, -c.ixz_kgm2, -c.iyz_kgm2});
    }

    // Shift the accumulated origin-referenced tensor down to the true
    // combined CG (the inverse of the per-contribution shift above, applied
    // once to the whole sum) and return the canonical result.
    RuntimeMassProperties finalize() const {
        RuntimeMassProperties result;
        result.mass_kg = mass_kg;
        if (mass_kg <= 0.0) return result;
        const double cx = mx / mass_kg, cy = my / mass_kg, cz = mz / mass_kg;
        result.cg_x_m = cx; result.cg_y_m = cy; result.cg_z_m = cz;
        result.ixx_kgm2 = std::max(0.0, ixx - mass_kg * (cy * cy + cz * cz));
        result.iyy_kgm2 = std::max(0.0, iyy - mass_kg * (cx * cx + cz * cz));
        result.izz_kgm2 = std::max(0.0, izz - mass_kg * (cx * cx + cy * cy));
        result.ixy_kgm2 = ixy + mass_kg * cx * cy;
        result.ixz_kgm2 = ixz + mass_kg * cx * cz;
        result.iyz_kgm2 = iyz + mass_kg * cy * cz;
        return result;
    }
};

}  // namespace runtime_mass
