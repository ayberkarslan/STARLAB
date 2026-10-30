#include "../export.hpp"
#include "interpolation.hpp"
// Production firing loop: a motor cannot produce thrust before its resolved ignition time.
STARLAB_EXPORT double motor_installed_thrust(const double* values,double count,double local){
 if(local<0.0)return 0.0;
 const auto c=starlab_motor::unpack(values,count);return starlab_motor::lerp(local,c.time,c.force);
}
