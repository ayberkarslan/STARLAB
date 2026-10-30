#pragma once
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
namespace starlab_motor {
struct Curve {std::vector<double> time,force;};
inline Curve unpack(const double* values,double count){
 Curve curve;if(!values||!(count>=2&&count<=10000)||count!=std::floor(count))return curve;
 for(int i=0;i<static_cast<int>(count);++i){curve.time.push_back(values[2*i]);curve.force.push_back(values[2*i+1]);}return curve;
}
}
