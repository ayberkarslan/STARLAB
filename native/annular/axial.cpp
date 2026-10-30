#include "../export.hpp"
#include <algorithm>
// RoketSiM design_analysis.hpp::analyzeComponent(Centering Ring / Bulkhead).
STARLAB_EXPORT double annular_axial(double mass,double radius,double innerRadius) {return 0.5*mass*(innerRadius*innerRadius+radius*radius);}
