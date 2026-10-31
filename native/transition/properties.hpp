#pragma once
#include <algorithm>
#include <cmath>

namespace StarlabTransition {
constexpr double pi=3.141592653589793238462643383279502884;
inline double frustumVolume(double r1,double r2,double length){return pi*length*(r1*r1+r1*r2+r2*r2)/3.0;}
inline double frustumCg(double r1,double r2,double length){const double d=r1*r1+r1*r2+r2*r2;return d<=0?0:length*(r1*r1+2*r1*r2+3*r2*r2)/(4*d);}
inline double volume(double length,double fore,double aft,double wall){return frustumVolume(fore,aft,length)-frustumVolume(std::max(0.0,fore-wall),std::max(0.0,aft-wall),length);}
inline double cg(double length,double fore,double aft,double wall){const double fi=std::max(0.0,fore-wall),ai=std::max(0.0,aft-wall),vo=frustumVolume(fore,aft,length),vi=frustumVolume(fi,ai,length),v=vo-vi;return v<=0?0:(vo*frustumCg(fore,aft,length)-vi*frustumCg(fi,ai,length))/v;}
}
