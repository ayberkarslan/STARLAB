#include "../export.hpp"
#include <algorithm>
#include "runtime_mass_properties.hpp"
STARLAB_EXPORT double assembly_origin(double mass,double x,double transverse) {
 runtime_mass::OriginAccumulator a;a.add({mass,x,0,0,0,transverse,transverse});return a.iyy;
}
