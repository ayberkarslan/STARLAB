#include "../export.hpp"
// Production analyzed motor transverse inertia / wet mass, used by runtime motor instances.
STARLAB_EXPORT double motor_inertia_coefficient(double wet,double radius,double length){
 const double singleTransverse=wet*(3.0*radius*radius+length*length)/12.0;
 return singleTransverse/wet;
}
