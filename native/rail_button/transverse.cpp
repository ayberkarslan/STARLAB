#include "../export.hpp"
STARLAB_EXPORT double rail_button_transverse(double mass,double height,double ro,double ri,double count,double separation){double value=mass*(3.0*(ro*ro+ri*ri)+height*height)/12.0,mean=(count-1.0)*separation*.5,one=mass/count;for(int i=0;i<static_cast<int>(count);++i){const double d=i*separation-mean;value+=one*d*d;}return value;}
