#include "../export.hpp"
#include "../static_aero/fin_barrowman.hpp"
#include "metrics.hpp"

STARLAB_EXPORT double freeform_static_cna(const double* coordinates,double count,double fins,double bodyRadius,double referenceArea,double mach,double alpha,double overrideEnabled,double overrideValue){
 if(overrideEnabled!=0.0)return overrideValue;
 const auto m=freeform_metrics(coordinates,count);
 if(!m.valid)return 0.0;
 return StarlabFinBarrowman::aggregate(m.area,m.span,m.midChordCosine,mach,alpha,referenceArea,static_cast<int>(std::lround(fins)),bodyRadius);
}
