#include "../export.hpp"
#include "metrics.hpp"

// Identical polygon area from RoketSiM FinPlanform::calculate.
STARLAB_EXPORT double freeform_planform_area(const double* coordinates,double count){
 const auto metrics=freeform_metrics(coordinates,count);
 return metrics.valid?metrics.area:0.0;
}
