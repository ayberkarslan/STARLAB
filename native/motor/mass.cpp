#include "../export.hpp"
// MotorModel::mass and the production dry/propellant contribution sum.
STARLAB_EXPORT double motor_mass(double dry,double remaining){return dry+remaining;}
