#include "../export.hpp"
#include <cmath>
// Source: RoketSiM src/components/InnerTube.cpp::volume.
// SI units; source +Z maps to STARLAB +X. Inertia is about the local CG.
STARLAB_EXPORT double inner_tube_volume(double radius, double innerRadius, double length) {
    const double pi = std::acos(-1.0);
    return pi * (radius * radius - innerRadius * innerRadius) * length;
}
