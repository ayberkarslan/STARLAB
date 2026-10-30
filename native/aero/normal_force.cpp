#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_normal_force(double coefficient,double pressure,double area){
 return coefficient*pressure*area;
}
