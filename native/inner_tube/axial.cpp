#include "../export.hpp"
#include <cmath>
// Source: RoketSiM src/components/InnerTube.cpp::rawMassProperties.
// SI units; source +Z maps to STARLAB +X. Inertia is about the local CG.
STARLAB_EXPORT double inner_tube_axial(double mass, double radius, double innerRadius) {
    return 0.5 * mass * (radius * radius + innerRadius * innerRadius);
}
