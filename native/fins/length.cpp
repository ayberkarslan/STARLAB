#include "../export.hpp"
#include <algorithm>
// RoketSiM analyzeComponent(Fins), extent used by its transverse inertia approximation.
STARLAB_EXPORT double fin_length(double root,double tip,double sweep) {return std::max(root,sweep+tip);}
