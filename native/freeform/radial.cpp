#include "../export.hpp"
#include "metrics.hpp"
STARLAB_EXPORT double freeform_structure_radial(const double* points,double count,double bodyRadius){return bodyRadius+freeform_metrics(points,count).centroidY;}
