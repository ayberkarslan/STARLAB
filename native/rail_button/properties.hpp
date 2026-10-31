#pragma once
#include <algorithm>
#include <cmath>
namespace StarlabRailButton {constexpr double pi=3.141592653589793238462643383279502884;inline double oneVolume(double height,double ro,double ri,double base,double flange,double screw){const double inner=std::max(0.0,height-base-flange);return pi*(ro*ro*(base+flange)+ri*ri*inner+(2.0/3.0)*ro*ro*screw);}}
