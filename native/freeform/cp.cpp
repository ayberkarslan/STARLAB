#include "../export.hpp"
#include "../static_aero/fin_barrowman.hpp"
#include "metrics.hpp"

STARLAB_EXPORT double freeform_static_cp(const double* coordinates,double count,double mach,double overrideEnabled,double overrideValue){
 if(overrideEnabled!=0.0)return overrideValue;
 const auto m=freeform_metrics(coordinates,count);
 if(!m.valid)return 0.0;
 const double aspect=2.0*m.span*m.span/m.area;
 return m.macLeadingEdge+StarlabFinBarrowman::cpFraction(aspect,mach)*m.macLength;
}
