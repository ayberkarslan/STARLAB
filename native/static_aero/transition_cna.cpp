#include "../export.hpp"
STARLAB_EXPORT double static_transition_cna(double,double fore,double aft,double referenceRadius,double overrideEnabled,double overrideValue,double,double){if(overrideEnabled!=0.0)return overrideValue;return 2.0*(aft*aft-fore*fore)/(referenceRadius*referenceRadius);}
