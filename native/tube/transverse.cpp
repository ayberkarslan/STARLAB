#include "../export.hpp"
// Source: RoketSiM src/components/BodyTube.cpp::rawMassProperties.
// Uniform tube transverse inertia (kg m²), about its own CG.
STARLAB_EXPORT double tube_transverse(double mass, double radius, double thickness, double length) {
    const double inner = radius - thickness;
    return (1.0 / 12.0) * mass * (3.0 * (radius * radius + inner * inner) + length * length);
}
