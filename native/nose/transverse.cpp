#include "../export.hpp"
#include "properties.hpp"
// Source: RoketSiM production design_analysis.hpp; inertia is about local CG.
STARLAB_EXPORT double nose_transverse(double length, double radius, double wall, double profile, double parameter, double filled, double shoulderLength, double shoulderRadius, double shoulderWall, double capped, double density) {
 return starlab_nose::calculate(length, radius, wall, density, profile, parameter, filled, shoulderLength, shoulderRadius, shoulderWall, capped).iTransverse;
}
