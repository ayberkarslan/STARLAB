#include "../export.hpp"
#include <algorithm>
// Original remainingPropellantKg boundary and final expression.
STARLAB_EXPORT double motor_remaining(double wet,double dry,double impulse,double totalImpulse,double local){
 const double initialPropellant=std::max(0.0,wet-dry);
 if(local<=0.0)return initialPropellant;
 return initialPropellant*std::clamp(1.0-impulse/std::max(totalImpulse,1e-12),0.0,1.0);
}
