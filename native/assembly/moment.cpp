#include "../export.hpp"
#include <algorithm>
#include "runtime_mass_properties.hpp"
// Canonical production OriginAccumulator::add, restricted to coaxial structure.
STARLAB_EXPORT double assembly_moment(double mass,double x) {
 runtime_mass::OriginAccumulator a;a.add({mass,x,0,0});return a.mx;
}
