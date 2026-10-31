#include <cmath>
#include "../export.hpp"
STARLAB_EXPORT double static_aero_centroid(double cna,double moment){return std::abs(cna)>1.0e-12?moment/cna:0.0;}
