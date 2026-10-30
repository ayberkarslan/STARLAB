#include "../export.hpp"
// Source: RoketSiM src/components/BodyTube.cpp::rawMassProperties.
// Homogeneous material mass (kg): bulk density × material volume.
STARLAB_EXPORT double tube_mass(double volume, double density) { return density * volume; }
