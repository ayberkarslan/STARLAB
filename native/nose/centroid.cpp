#include "../export.hpp"
#include "properties.hpp"
// Source: RoketSiM production design_analysis.hpp; inertia is about local CG.
STARLAB_EXPORT double nose_cg(double length, double radius, double wall, double profile, double parameter, double filled, double shoulderLength, double shoulderRadius, double shoulderWall, double capped) {
 return starlab_nose::calculate(length, radius, wall, 1.0, profile, parameter, filled, shoulderLength, shoulderRadius, shoulderWall, capped).localCg;
}
