#include "../export.hpp"
#include "metrics.hpp"
STARLAB_EXPORT double freeform_structure_length(const double* points,double count){return freeform_metrics(points,count).axialLength;}
