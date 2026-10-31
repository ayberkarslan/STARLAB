#include <cmath>
#include "../export.hpp"
#include "fin_barrowman.hpp"

STARLAB_EXPORT double static_fin_cna(
 double root,double tip,double span,double sweep,double count,double bodyRadius,
 double referenceArea,double mach,double alpha,double overrideEnabled,double overrideValue,double,double
){
 if(overrideEnabled!=0.0)return overrideValue;
 const double area=.5*(root+tip)*span;
 const double cosine=span/std::max(std::hypot(span,sweep+.5*(tip-root)),1.0e-12);
 return StarlabFinBarrowman::aggregate(area,span,cosine,mach,alpha,referenceArea,static_cast<int>(std::lround(count)),bodyRadius);
}
