#include "../export.hpp"
// Production design_analysis.hpp Motor: fallback when no measured CG curve exists.
STARLAB_EXPORT double motor_cg(double length){return length/2.0;}
