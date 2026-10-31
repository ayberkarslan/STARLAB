#include <cmath>
#include <vector>
#include "../export.hpp"
#include "fin_planform.hpp"

// Identical polygon area from RoketSiM FinPlanform::calculate.
STARLAB_EXPORT double freeform_planform_area(const double* coordinates,double count){
 if(!coordinates||!std::isfinite(count)||count<3||count>500||std::floor(count)!=count)return 0.0;
 std::vector<FinPlanform::Point> points;
 points.reserve(static_cast<std::size_t>(count));
 for(int i=0;i<static_cast<int>(count);++i){
  const double x=coordinates[2*i],y=coordinates[2*i+1];
  if(!std::isfinite(x)||!std::isfinite(y))return 0.0;
  points.push_back({x,y});
 }
 const auto metrics=FinPlanform::calculate(points);
 return metrics.valid?metrics.area:0.0;
}
