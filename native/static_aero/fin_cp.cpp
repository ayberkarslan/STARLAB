#include <algorithm>
#include "../export.hpp"
#include "fin_barrowman.hpp"

STARLAB_EXPORT double static_fin_cp(
 double root,double tip,double span,double sweep,double,double,
 double,double mach,double,double,double,double overrideEnabled,double overrideValue
){
 if(overrideEnabled!=0.0)return overrideValue;
 const double sum=root+tip,area=.5*sum*span;
 if(sum<=1.0e-12||area<=1.0e-12)return 0.0;
 const double aspect=2.0*span*span/area;
 const double mac=2.0*(root*root+root*tip+tip*tip)/(3.0*sum);
 const double lead=sweep*(root+2.0*tip)/(3.0*sum);
 return lead+StarlabFinBarrowman::cpFraction(aspect,mach)*mac;
}
