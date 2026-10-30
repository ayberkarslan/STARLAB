#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_effective_alpha(double alpha){
 const double stallAngle=20.0*3.14159265358979323846/180.0;return std::clamp(alpha,-stallAngle,stallAngle);
}
