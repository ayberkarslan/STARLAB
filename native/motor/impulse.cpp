#include "../export.hpp"
#include "integrals.hpp"
STARLAB_EXPORT double motor_impulse(const double* values,double count,double time){return starlab_motor::integrals(starlab_motor::unpack(values,count),time).first;}
