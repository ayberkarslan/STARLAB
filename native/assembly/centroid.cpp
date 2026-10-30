#include "../export.hpp"
#include <algorithm>
#include "runtime_mass_properties.hpp"
STARLAB_EXPORT double assembly_cg(double mass,double moment) {
 runtime_mass::OriginAccumulator a;a.mass_kg=mass;a.mx=moment;return a.finalize().cg_x_m;
}
