#include "../export.hpp"
#include "integrals.hpp"
STARLAB_EXPORT double motor_total_impulse(const double* values,double count){return starlab_motor::integrals(starlab_motor::unpack(values,count),0.0).second;}
