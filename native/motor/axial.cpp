#include "../export.hpp"
// Production runtime motor records carry no axial inertia. Preserve that documented limitation.
STARLAB_EXPORT double motor_axial(){return 0.0;}
