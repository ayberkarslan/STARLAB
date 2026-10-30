#include "../export.hpp"
#include <algorithm>
// RoketSiM design_analysis.hpp::analyzeComponent(Centering Ring / Bulkhead).
STARLAB_EXPORT double annular_volume(double radius,double innerRadius,double length) {const double kPi=3.14159265358979323846; return kPi*std::max(radius*radius-innerRadius*innerRadius,0.0)*length;}
