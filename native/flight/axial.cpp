#include "../export.hpp"
#include <algorithm>
#include <cmath>

extern "C" double motor_thrust(const double*,double,double);
extern "C" double motor_impulse(const double*,double,double);
extern "C" double motor_total_impulse(const double*,double);
extern "C" double environment_temperature(double,double,double,double);
extern "C" double environment_pressure(double,double,double,double,double,double);
extern "C" double environment_density(double,double);
extern "C" double environment_gravity(double);
extern "C" double environment_wind_east(double,double);
extern "C" double environment_wind_north(double,double);
extern "C" double environment_wind_along(double,double,double);
extern "C" double aero_axial_force(double,double,double,double,double);

namespace {
struct State { double x=0.0,z=0.0,vx=0.0,vz=0.0,railCleared=0.0; };
struct Derivative { double x=0.0,z=0.0,vx=0.0,vz=0.0; };
struct Context {
 const double* curve;double pairs,wet,propellant,cd,area,atmosphere,launchAltitude,surfaceTemperature,surfacePressure,windAlong,angle;
 double railLength,totalImpulse;
};
double massAt(const Context& c,double t){
 const double consumed=std::clamp(motor_impulse(c.curve,c.pairs,t)/std::max(c.totalImpulse,1e-12),0.0,1.0);
 return c.wet-c.propellant*consumed;
}
Derivative derivative(double t,const State& s,const Context& c){
 const double altitude=std::max(0.0,s.z);
 const double absoluteAltitude=c.launchAltitude+altitude;
 const double temperature=environment_temperature(c.atmosphere,altitude,absoluteAltitude,c.surfaceTemperature);
 const double pressure=environment_pressure(c.atmosphere,altitude,absoluteAltitude,c.surfaceTemperature,c.surfacePressure,temperature);
 const double density=environment_density(pressure,temperature);
 const double airX=s.vx-c.windAlong,airZ=s.vz,airSpeed=std::hypot(airX,airZ);
 const double dynamicPressure=.5*density*airSpeed*airSpeed,drag=aero_axial_force(airSpeed,0.0,dynamicPressure,c.area,c.cd);
 const double mass=massAt(c,t),gravity=environment_gravity(absoluteAltitude);
 const double bx=std::sin(c.angle),bz=std::cos(c.angle),thrust=motor_thrust(c.curve,c.pairs,t);
 double ax=(thrust*bx+(airSpeed>0.1?drag*airX/airSpeed:0.0))/mass;
 double az=(thrust*bz+(airSpeed>0.1?drag*airZ/airSpeed:0.0))/mass-gravity;
 if(s.railCleared<.5){
  double along=ax*bx+az*bz;const double distance=std::max(0.0,s.x*bx+s.z*bz);
  if(distance<=0.0&&s.vx*bx+s.vz*bz<=0.0&&along<0.0)along=0.0;
  ax=along*bx;az=along*bz;
 }
 return {s.vx,s.vz,ax,az};
}
State advance(double t,const State& s,double dt,const Context& c){
 const auto k1=derivative(t,s,c);
 const State s2{s.x+k1.x*dt/2,s.z+k1.z*dt/2,s.vx+k1.vx*dt/2,s.vz+k1.vz*dt/2,s.railCleared};const auto k2=derivative(t+dt/2,s2,c);
 const State s3{s.x+k2.x*dt/2,s.z+k2.z*dt/2,s.vx+k2.vx*dt/2,s.vz+k2.vz*dt/2,s.railCleared};const auto k3=derivative(t+dt/2,s3,c);
 const State s4{s.x+k3.x*dt,s.z+k3.z*dt,s.vx+k3.vx*dt,s.vz+k3.vz*dt,s.railCleared};const auto k4=derivative(t+dt,s4,c);
 State next{s.x+dt*(k1.x+2*k2.x+2*k3.x+k4.x)/6,s.z+dt*(k1.z+2*k2.z+2*k3.z+k4.z)/6,s.vx+dt*(k1.vx+2*k2.vx+2*k3.vx+k4.vx)/6,s.vz+dt*(k1.vz+2*k2.vz+2*k3.vz+k4.vz)/6,s.railCleared};
 const double bx=std::sin(c.angle),bz=std::cos(c.angle);
 if(next.railCleared<.5&&next.x*bx+next.z*bz>=c.railLength)next.railCleared=1.0;
 if(next.railCleared<.5&&next.z<0.0){next.x=next.z=next.vx=next.vz=0.0;}return next;
}
}

// Reduced-order single-stage flight from RoketSiM's production thrust, mass,
// atmosphere, gravity, drag, pad support and RK4 equations. Output rows are
// time, AGL altitude, path speed and path acceleration.
STARLAB_EXPORT double flight_axial_solve(const double* curve,double pairs,double wetMass,double propellantMass,double cd,double area,double atmosphereModel,double launchAltitude,double surfaceTemperature,double surfacePressure,double windSpeed,double windDirection,double heading,double railAngle,double railLength,double dt,double maxTime,double* output,double capacity){
 if(pairs<2||wetMass<=0||propellantMass<0||propellantMass>wetMass||cd<0||area<=0||railLength<=0||dt<=0||maxTime<=0||maxTime/dt>2000000.0||capacity<8)return 0.0;
 const double pi=3.14159265358979323846;
 const double windAlong=environment_wind_along(environment_wind_east(windSpeed,windDirection),environment_wind_north(windSpeed,windDirection),heading);
 const Context c{curve,pairs,wetMass,propellantMass,cd,area,atmosphereModel,launchAltitude,surfaceTemperature,surfacePressure,windAlong,railAngle*pi/180.0,railLength,motor_total_impulse(curve,pairs)};
 const int maxRows=static_cast<int>(capacity)/4,totalSteps=static_cast<int>(std::ceil(maxTime/dt)),stride=std::max(1,static_cast<int>(std::ceil((totalSteps+1.0)/maxRows)));
 State state{};double t=0.0;int rows=0;bool lifted=false;
 auto emit=[&](){if(rows>=maxRows)return;const auto d=derivative(t,state,c);output[4*rows]=t;output[4*rows+1]=state.z;output[4*rows+2]=std::hypot(state.vx,state.vz);output[4*rows+3]=std::hypot(d.vx,d.vz);++rows;};
 emit();
 for(int step=1;step<=totalSteps&&rows<maxRows;++step){const double h=std::min(dt,maxTime-t);state=advance(t,state,h,c);t+=h;lifted=lifted||state.z>1e-6;if(lifted&&state.z<=0.0){state.z=state.vx=state.vz=0.0;}if(step%stride==0||t>=maxTime||(lifted&&state.z<=0.0)){emit();if(lifted&&state.z<=0.0)break;}if(h<=0.0)break;}
 return static_cast<double>(rows);
}
