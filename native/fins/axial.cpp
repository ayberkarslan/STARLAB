#include "../export.hpp"
// RoketSiM analyzeComponent(Fins), production axial inertia approximation.
STARLAB_EXPORT double fin_axial(double mass,double radial) {return mass*radial*radial;}
