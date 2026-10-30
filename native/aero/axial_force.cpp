#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_axial_force(double speed,double alpha,double pressure,double area,double cd){
 const double axialCd=speed>0.1?cd:0.0;const double v_ax=speed*std::cos(alpha);const double direction=v_ax>=0.0?1.0:-1.0;return -direction*pressure*area*axialCd;
}
