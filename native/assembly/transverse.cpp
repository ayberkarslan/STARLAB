#include "../export.hpp"
#include <algorithm>
#include "runtime_mass_properties.hpp"
STARLAB_EXPORT double assembly_transverse(double mass,double moment,double origin) {
 runtime_mass::OriginAccumulator a;a.mass_kg=mass;a.mx=moment;a.iyy=origin;return a.finalize().iyy_kgm2;
}
