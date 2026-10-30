#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_dynamic_pressure(double density,double speed){
 return 0.5*density*speed*speed;
}
