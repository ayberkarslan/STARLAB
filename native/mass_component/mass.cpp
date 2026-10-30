#include "../export.hpp"
#include <cmath>
// Source: RoketSiM src/components/MassComponent.cpp::rawMassProperties.
// SI units; source +Z maps to STARLAB +X. Inertia is about the local CG.
STARLAB_EXPORT double mass_component_mass(double suppliedMass) {
    return suppliedMass;
}
