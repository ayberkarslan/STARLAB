#include "../export.hpp"
#include <algorithm>
#include <cmath>
// RoketSiM production CLI aerodynamic force/moment evaluation; source assumptions preserved.
STARLAB_EXPORT double aero_body_cn(double mach,double alpha,double bodyArea,double area){
 double bodyLiftMultiplier=1.0;if(mach<0.05&&std::abs(alpha)>3.14159265358979323846/4.0)bodyLiftMultiplier=std::pow(mach/0.05,2.0);return bodyLiftMultiplier*1.1*bodyArea/std::max(area,1e-12)*std::sin(alpha)*std::abs(std::sin(alpha));
}
