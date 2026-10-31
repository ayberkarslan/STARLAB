#pragma once
#include <algorithm>
#include <cmath>

namespace StarlabFinBarrowman {
constexpr double pi=3.141592653589793238462643383279502884;
constexpr double gamma=1.4;
constexpr double stall=20.0*pi/180.0;

inline double limitedAlpha(double angle){
 double alpha=std::fmod(std::abs(angle),2.0*pi);if(alpha>pi)alpha=2.0*pi-alpha;
 return std::min({alpha,pi-alpha,stall});
}
struct Coefficients{double k1,k2,k3;};
inline Coefficients exact(double mach){
 const double beta=std::sqrt(std::max(mach*mach-1.0,1.0e-14)),b2=beta*beta,m2=mach*mach,m4=m2*m2,m6=m4*m2,m8=m4*m4;
 return {2.0/beta,((gamma+1.0)*m4-4.0*b2)/(4.0*b2*b2),((gamma+1.0)*m8+(2.0*gamma*gamma-7.0*gamma-5.0)*m6+10.0*(gamma+1.0)*m4+8.0)/(6.0*std::pow(beta,7.0))};
}
inline Coefficients coefficients(double mach){
 const double bounded=std::clamp(mach,1.5,4.9),position=(bounded-1.5)*10.0;
 const int lower=std::clamp(static_cast<int>(std::floor(position+1.0e-12)),0,34),upper=std::min(lower+1,34);
 const double lowerMach=1.5+0.1*lower,upperMach=1.5+0.1*upper;const auto a=exact(lowerMach);if(lower==upper)return a;const auto b=exact(upperMach);const double f=std::clamp((bounded-lowerMach)/(upperMach-lowerMach),0.0,1.0);
 return {a.k1+f*(b.k1-a.k1),a.k2+f*(b.k2-a.k2),a.k3+f*(b.k3-a.k3)};
}
inline double quartic(double x,double x0,double x1,double v0,double v1,double d0,double d1){
 const double interval=x1-x0,t=std::clamp((x-x0)/interval,0.0,1.0),a0=v0,a1=interval*d0,difference=v1-a0-a1,dd=interval*d1-a1,a3=4.0*difference-dd,a4=dd-3.0*difference;
 return a0+t*(a1+t*t*(a3+t*a4));
}
inline double oneFin(double area,double span,double cosine,double mach,double angle,double referenceArea){
 if(area<=1.0e-14||span<=1.0e-14||cosine<=1.0e-14||referenceArea<=1.0e-14)return 0.0;
 const double alpha=limitedAlpha(angle),areaCosine=area*cosine;
 auto sub=[&](double m){const double ratio=span*span/areaCosine,root=std::sqrt(std::max(1.0+(1.0-m*m)*ratio*ratio,0.0));return 2.0*pi*span*span/(referenceArea*(1.0+root));};
 auto sup=[&](double m){const auto c=coefficients(m);return area/referenceArea*(c.k1+c.k2*alpha+c.k3*alpha*alpha);};
 if(mach<=0.9)return sub(mach);if(mach>=1.5)return sup(mach);
 const double ratio=span*span/areaCosine,root=std::sqrt(std::max(1.0+(1.0-0.9*0.9)*ratio*ratio,0.0));
 const double subDerivative=2.0*mach*pi*std::pow(span,6.0)/(areaCosine*areaCosine*referenceArea*root*(1.0+root)*(1.0+root));
 const double superB=std::pow(1.5*1.5-1.0,1.5),superDerivative=-area/referenceArea*2.0*1.5/superB;
 return quartic(mach,0.9,1.5,sub(0.9),sup(1.5),subDerivative,superDerivative);
}
inline double orientationSum(int count){double sum=0;for(int i=0;i<count;++i){const double s=std::sin(pi/2.0-2.0*pi*i/count);sum+=s*s;}return sum;}
inline double interference(int count){if(count==5)return .948;if(count==6)return .913;if(count==7)return .854;if(count==8)return .81;return count>8?.75:1.0;}
inline double aggregate(double area,double span,double cosine,double mach,double angle,double referenceArea,int count,double bodyRadius){
 const double tau=bodyRadius>0?bodyRadius/std::max(bodyRadius+span,1.0e-14):0.0;
 return oneFin(area,span,cosine,mach,angle,referenceArea)*orientationSum(count)*interference(count)*(1.0+tau);
}
inline double cpFraction(double aspectRatio,double mach){
 if(mach<=.5)return .25;
 auto sup=[&](double m){const double beta=std::sqrt(std::max(m*m-1.0,0.0));double d=2.0*aspectRatio*beta-1.0;if(std::abs(d)<1.0e-12)d=std::copysign(1.0e-12,d);return(aspectRatio*beta-.67)/d;};
 if(mach>=2.0)return sup(mach);
 const double d=std::pow(1.0-3.4641*aspectRatio,2.0);if(d<=1.0e-20)return .25;const double ar=aspectRatio;
 const double c[6]={9.16049*(-.588838+ar)*(-.20624+ar)/d,-31.6049*(-.705375+ar)*(-.198476+ar)/d,55.3086*(-.711482+ar)*(-.196772+ar)/d,-39.5062*(-.72074+ar)*(-.194245+ar)/d,12.8395*(-.725688+ar)*(-.19292+ar)/d,-1.58025*(-.728769+ar)*(-.192105+ar)/d};
 double value=0,power=1;for(double coefficient:c){value+=coefficient*power;power*=mach;}return value;
}
}
