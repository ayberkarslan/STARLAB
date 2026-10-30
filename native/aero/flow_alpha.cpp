#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_flow_alpha(double speed,double alpha){
 return speed>0.1?alpha:0.0;
}
