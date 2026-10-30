#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_body_cp(double bodyArea,double bodyMoment,double cp){
 return bodyArea>1e-12?bodyMoment/bodyArea:cp;
}
