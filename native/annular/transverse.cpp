#include "../export.hpp"
#include <algorithm>
// RoketSiM design_analysis.hpp::analyzeComponent(Centering Ring / Bulkhead).
STARLAB_EXPORT double annular_transverse(double mass,double radius,double innerRadius,double length) {return mass*(3.0*(innerRadius*innerRadius+radius*radius)+length*length)/12.0;}
