#include "../export.hpp"
// Original CLI local = t - resolved_ignition_time_s.
STARLAB_EXPORT double motor_local_time(double time,double delay){return time-delay;}
