#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_lever(double cp,double cg){
 return cp-cg;
}
