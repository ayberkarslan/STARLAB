#pragma once
// C++14. SI units, body x forward/y right/z down, Earth NED.
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace flight {
constexpr double gravity=9.80665, pi=3.14159265358979323846;
inline void require(bool ok,const std::string& why){if(!ok) throw std::runtime_error(why);}
struct Vec {double x=0,y=0,z=0;};
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(double k,Vec a){return {k*a.x,k*a.y,k*a.z};}
inline double dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(Vec a){return std::sqrt(dot(a,a));}
struct Quat {double s=1; Vec v;};
inline Quat operator+(Quat a,Quat b){return {a.s+b.s,a.v+b.v};}
inline Quat operator*(double k,Quat a){return {k*a.s,k*a.v};}
inline Quat product(Quat a,Quat b){return {a.s*b.s-dot(a.v,b.v),a.s*b.v+b.s*a.v+cross(a.v,b.v)};}
inline Quat unit(Quat a){double n=std::sqrt(a.s*a.s+dot(a.v,a.v));require(n>0&&std::isfinite(n),"Invalid orientation");return (1/n)*a;}
inline Vec rotate(Quat a,Vec v){Vec t=2*cross(a.v,v);return v+a.s*t+cross(a.v,t);}
inline Vec inverseRotate(Quat a,Vec v){return rotate({a.s,-1*a.v},v);}
} // namespace flight