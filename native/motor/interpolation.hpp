#pragma once
#include "buffer.hpp"
namespace starlab_motor {
// Original production interpolation body, including endpoint clamping.
static double lerp(double x,
                   const std::vector<double>& xp,
                   const std::vector<double>& yp)
{
    if (xp.empty()) return 0.0;
    if (x <= xp.front()) return yp.front();
    if (x >= xp.back())  return yp.back();
    for (size_t i = 0; i < xp.size() - 1; ++i) {
        if (x >= xp[i] && x <= xp[i+1]) {
            double t = (x - xp[i]) / (xp[i+1] - xp[i]);
            return yp[i] + t * (yp[i+1] - yp[i]);
        }
    }
    return yp.back();
}


}
