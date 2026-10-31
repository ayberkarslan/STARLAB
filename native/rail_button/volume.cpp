#include "../export.hpp"
#include "properties.hpp"
STARLAB_EXPORT double rail_button_volume(double height,double ro,double ri,double base,double flange,double screw,double count,double){return StarlabRailButton::oneVolume(height,ro,ri,base,flange,screw)*count;}
