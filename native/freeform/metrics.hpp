#pragma once
#include <cmath>
#include <vector>
#include "fin_planform.hpp"

inline FinPlanform::Metrics freeform_metrics(const double* coordinates,double count){
 if(!coordinates||!std::isfinite(count)||count<3||count>500||std::floor(count)!=count)return {};
 std::vector<FinPlanform::Point> points;
 points.reserve(static_cast<std::size_t>(count));
 for(int i=0;i<static_cast<int>(count);++i){
  const double x=coordinates[2*i],y=coordinates[2*i+1];
  if(!std::isfinite(x)||!std::isfinite(y))return {};
  points.push_back({x,y});
 }
 return FinPlanform::calculate(points);
}
