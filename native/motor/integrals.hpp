#pragma once
#include "buffer.hpp"
namespace starlab_motor {
// Original remainingPropellantKg loop, separated into consumed/total impulse.
inline std::pair<double,double> integrals(const Curve& curve,double local){
 double impulse=0.0,totalImpulse=0.0;
 for(std::size_t i=1;i<curve.time.size();++i){
  const double t0=curve.time[i-1],t1=curve.time[i];
  const double segment=0.5*(curve.force[i-1]+curve.force[i])*std::max(t1-t0,0.0);
  totalImpulse+=segment;
  if(local>t0){
   const double end=std::min(local,t1);
   const double ratio=(end-t0)/std::max(t1-t0,1e-12);
   const double fEnd=curve.force[i-1]+ratio*(curve.force[i]-curve.force[i-1]);
   impulse+=0.5*(curve.force[i-1]+fEnd)*std::max(end-t0,0.0);
  }
 }
 return {impulse,totalImpulse};
}
}
