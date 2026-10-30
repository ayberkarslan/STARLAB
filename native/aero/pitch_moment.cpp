#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_pitch_moment(double pressure,double area,double normal,double lever,double bodyCn,double bodyCp,double cg){
 return -pressure*area*(normal*lever+bodyCn*(bodyCp-cg));
}
