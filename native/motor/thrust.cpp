#include "../export.hpp"
#include "interpolation.hpp"
STARLAB_EXPORT double motor_thrust(const double* values,double count,double time){
 const auto c=starlab_motor::unpack(values,count);return starlab_motor::lerp(time,c.time,c.force);
}
