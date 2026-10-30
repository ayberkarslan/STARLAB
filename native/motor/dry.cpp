#include "../export.hpp"
// Production dry mass = wet mass - initial propellant.
STARLAB_EXPORT double motor_dry(double wet,double propellant){return wet-propellant;}
