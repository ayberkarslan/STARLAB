#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_stability_margin(double lever,double diameter){
 return diameter>0.0?lever/diameter:0.0;
}
