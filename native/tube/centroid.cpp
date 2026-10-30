#include "../export.hpp"
// Source: RoketSiM src/components/BodyTube.cpp::rawMassProperties.
// Uniform tube local CG (m) from the leading end along +X.
STARLAB_EXPORT double tube_cg(double length) { return length / 2.0; }
