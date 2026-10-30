#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_normal_coefficient(double cna,double alpha,double bodyCn){
 return cna*alpha+bodyCn;
}
