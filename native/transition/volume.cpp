#include "../export.hpp"
#include "properties.hpp"
STARLAB_EXPORT double transition_volume(double length,double fore,double aft,double wall){return StarlabTransition::volume(length,fore,aft,wall);}
