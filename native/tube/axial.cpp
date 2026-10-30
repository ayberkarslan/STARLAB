#include "../export.hpp"
// Uniform tube axial inertia (kg m²), about its own CG; STARLAB +X.
STARLAB_EXPORT double tube_axial(double mass, double radius, double thickness) {
    const double inner = radius - thickness;
    return 0.5 * mass * (radius * radius + inner * inner);
}
