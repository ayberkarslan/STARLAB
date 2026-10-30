#include "../export.hpp"
#include <cmath>
// Source: RoketSiM src/components/MassComponent.cpp::rawMassProperties.
// SI units; source +Z maps to STARLAB +X. Inertia is about the local CG.
STARLAB_EXPORT double mass_component_transverse(double mass, double radius, double length) {
    return (1.0 / 12.0) * mass * (3.0 * radius * radius + length * length);
}
