#include "../export.hpp"
#include <cmath>
// Source: RoketSiM src/components/InnerTube.cpp::rawMassProperties.
// SI units; source +Z maps to STARLAB +X. Inertia is about the local CG.
STARLAB_EXPORT double inner_tube_transverse(double mass, double radius, double innerRadius, double length) {
    return (1.0 / 12.0) * mass * (3.0 * (radius * radius + innerRadius * innerRadius) + length * length);
}
