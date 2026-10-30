#include "../export.hpp"
// Production evaluateMassProperties: separate dry/fuel transverse contributions.
STARLAB_EXPORT double motor_transverse(double dry,double remaining,double coefficient){return dry*coefficient+remaining*coefficient;}
