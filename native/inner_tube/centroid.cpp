#include "../export.hpp"
#include <cmath>
// Source: RoketSiM src/components/InnerTube.cpp::rawMassProperties.
// SI units; source +Z maps to STARLAB +X. Inertia is about the local CG.
STARLAB_EXPORT double inner_tube_cg(double length) {
    return length / 2.0;
}
