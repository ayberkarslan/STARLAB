#include "../export.hpp"
// RoketSiM analyzeComponent(Fins). Production CG does not include a root-tab shift.
STARLAB_EXPORT double fin_cg(double root,double tip,double sweep) {
 const double denominator=3.0*(root+tip);
 return denominator>1e-12?(root*root+root*tip+tip*tip+sweep*(root+2.0*tip))/denominator:0.0;
}
