#include "../export.hpp"
// RoketSiM analyzeComponent(Fins), production transverse inertia approximation.
STARLAB_EXPORT double fin_transverse(double mass,double length,double radial) {return mass*(length*length+radial*radial)/12.0;}
