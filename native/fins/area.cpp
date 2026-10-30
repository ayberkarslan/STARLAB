#include "../export.hpp"
// RoketSiM src/cli/design_analysis.hpp::analyzeComponent(Fins), trapezoidal area.
STARLAB_EXPORT double fin_area(double root, double tip, double span) {return 0.5 * (root + tip) * span;}
