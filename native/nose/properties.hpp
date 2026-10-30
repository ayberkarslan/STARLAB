#pragma once
#include "shoulder.hpp"
#include <limits>
namespace starlab_nose {
// Scalar SI adapter around the original production algorithms. No JSON/UI dependency.
inline Properties calculate(double length,double radius,double wall,double density,double profile,double parameter,double filled,double shoulderLength,double shoulderRadius,double shoulderWall,double capped) {
 if(!std::isfinite(profile)||profile<0||profile>5||profile!=std::floor(profile)) {
  const double invalid=std::numeric_limits<double>::quiet_NaN();
  return {length,invalid,invalid,invalid,invalid,invalid};
 }
 const char* shapes[]={"Conical","Ogive","Ellipsoid","Parabolic","Power","Haack"};
 Properties result;result.length=length;
 integrateShell(length,filled?radius:wall,density,[&](double z){return noseRadius(shapes[static_cast<int>(profile)],z,length,radius,parameter);},result);
 addShoulder(result,density,shoulderLength,shoulderRadius,shoulderWall,capped!=0);
 return result;
}
}
