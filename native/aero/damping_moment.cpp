#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_damping_moment(double speed,double pressure,double area,double cna,double lever,double diameter,double pitchRate){
 if(speed<=0.1)return 0.0;const double dampingLever=std::max(std::abs(lever),0.25*diameter);return -4.0*pressure*area*cna*dampingLever*dampingLever*pitchRate/speed;
}
