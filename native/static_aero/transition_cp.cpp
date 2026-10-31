#include "../export.hpp"
STARLAB_EXPORT double static_transition_cp(double length,double fore,double aft,double,double,double,double overrideEnabled,double overrideValue){if(overrideEnabled!=0.0)return overrideValue;const double sum=fore+aft;return length<=0||sum<=0?0:length*(fore+2.0*aft)/(3.0*sum);}
