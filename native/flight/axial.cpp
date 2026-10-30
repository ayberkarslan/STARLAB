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
struct State { double distance=0.0,velocity=0.0; };
struct Derivative { double distance=0.0,velocity=0.0; };
struct Context {
 const double* curve;double pairs,wet,propellant,cd,area,atmosphere,launchAltitude,surfaceTemperature,surfacePressure,windAlong,angle;
 double totalImpulse;
};
double massAt(const Context& c,double t){
 const double consumed=std::clamp(motor_impulse(c.curve,c.pairs,t)/std::max(c.totalImpulse,1e-12),0.0,1.0);
 return c.wet-c.propellant*consumed;
}
Derivative derivative(double t,const State& s,const Context& c){
 const double altitude=std::max(0.0,s.distance*std::cos(c.angle));
 const double absoluteAltitude=c.launchAltitude+altitude;
 const double temperature=environment_temperature(c.atmosphere,altitude,absoluteAltitude,c.surfaceTemperature);
 const double pressure=environment_pressure(c.atmosphere,altitude,absoluteAltitude,c.surfaceTemperature,c.surfacePressure,temperature);
 const double density=environment_density(pressure,temperature);
 const double airSpeed=s.velocity-c.windAlong*std::sin(c.angle);
 const double dynamicPressure=.5*density*airSpeed*airSpeed;
 const double drag=aero_axial_force(std::abs(airSpeed),0.0,dynamicPressure,c.area,c.cd)*(airSpeed>=0.0?1.0:-1.0);
 const double mass=massAt(c,t),gravity=environment_gravity(absoluteAltitude);
 double acceleration=(motor_thrust(c.curve,c.pairs,t)+drag-mass*gravity*std::cos(c.angle))/mass;
 if(s.distance<=0.0&&s.velocity<=0.0&&acceleration<0.0)acceleration=0.0;
 return {s.velocity,acceleration};
}
State advance(double t,const State& s,double dt,const Context& c){
 const auto k1=derivative(t,s,c);
 const State s2{s.distance+k1.distance*dt/2,s.velocity+k1.velocity*dt/2};const auto k2=derivative(t+dt/2,s2,c);
 const State s3{s.distance+k2.distance*dt/2,s.velocity+k2.velocity*dt/2};const auto k3=derivative(t+dt/2,s3,c);
 const State s4{s.distance+k3.distance*dt,s.velocity+k3.velocity*dt};const auto k4=derivative(t+dt,s4,c);
 State next{s.distance+dt*(k1.distance+2*k2.distance+2*k3.distance+k4.distance)/6,s.velocity+dt*(k1.velocity+2*k2.velocity+2*k3.velocity+k4.velocity)/6};
 if(next.distance<0.0){next.distance=0.0;next.velocity=0.0;}return next;
}
}

// Reduced-order single-stage flight from RoketSiM's production thrust, mass,
// atmosphere, gravity, drag, pad support and RK4 equations. Output rows are
// time, AGL altitude, path speed and path acceleration.
STARLAB_EXPORT double flight_axial_solve(const double* curve,double pairs,double wetMass,double propellantMass,double cd,double area,double atmosphereModel,double launchAltitude,double surfaceTemperature,double surfacePressure,double windSpeed,double windDirection,double heading,double railAngle,double dt,double maxTime,double* output,double capacity){
 if(pairs<2||wetMass<=0||propellantMass<0||propellantMass>wetMass||cd<0||area<=0||dt<=0||maxTime<=0||maxTime/dt>2000000.0||capacity<8)return 0.0;
 const double pi=3.14159265358979323846;
 const double windAlong=environment_wind_along(environment_wind_east(windSpeed,windDirection),environment_wind_north(windSpeed,windDirection),heading);
 const Context c{curve,pairs,wetMass,propellantMass,cd,area,atmosphereModel,launchAltitude,surfaceTemperature,surfacePressure,windAlong,railAngle*pi/180.0,motor_total_impulse(curve,pairs)};
 const int maxRows=static_cast<int>(capacity)/4,totalSteps=static_cast<int>(std::ceil(maxTime/dt)),stride=std::max(1,static_cast<int>(std::ceil((totalSteps+1.0)/maxRows)));
 State state{};double t=0.0;int rows=0;bool lifted=false;
 auto emit=[&](){if(rows>=maxRows)return;const auto d=derivative(t,state,c);output[4*rows]=t;output[4*rows+1]=state.distance*std::cos(c.angle);output[4*rows+2]=state.velocity;output[4*rows+3]=d.velocity;++rows;};
 emit();
 for(int step=1;step<=totalSteps&&rows<maxRows;++step){const double h=std::min(dt,maxTime-t);state=advance(t,state,h,c);t+=h;lifted=lifted||state.distance>1e-6;if(step%stride==0||t>=maxTime||(lifted&&state.distance<=0.0)){emit();if(lifted&&state.distance<=0.0)break;}if(h<=0.0)break;}
 return static_cast<double>(rows);
}
