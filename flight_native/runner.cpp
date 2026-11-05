#include "vendor/flight.hpp"
#include <cstdlib>
#include <limits>

using namespace flight;

struct ProjectHost final : geometry::Host {
 geometry::Design design;
 const Phase* phase=nullptr;
 double site=0,windNorth=0,windEast=0,windDown=0,exitArea=0;bool legacy=false;
 const geometry::Design& activeDesign(const geometry::Request&) const override { return design; }
 geometry::Conditions conditions(const geometry::Request& r) const override {
  require(phase!=nullptr,"Missing flight phase");
  auto air=atmosphere(site-r.position.z,legacy);
  const double fraction=phase->burn>0?std::max(0.,std::min(1.,r.phaseTime/phase->burn)):1.;
  geometry::Conditions c{};
  c.rho=air.rho;c.soundSpeed=air.a;c.mu=air.mu;c.temperature=air.T;c.pressure=air.P;
  c.mass=phase->m0+fraction*(phase->m1-phase->m0);
  c.cgStation=-phase->cg;c.groundVelocityB=r.velocityB;
  Vec wind=phase->weather.empty()?Vec{windNorth,windEast,windDown}:sampleCurve(phase->weather,r.phaseTime,&WindPoint::wind);
  c.windB=inverseRotate(r.orientation,wind);
  c.gravityB=inverseRotate(r.orientation,{0,0,gravity});
  c.firing=r.firing;c.motorExitArea=exitArea;
  c.thrustB={r.firing?sampleCurve(phase->motor,r.phaseTime,&MotorPoint::force):0.,0,0};
  return c;
 }
};

template<class T> void read(T& value,const char* field){require(bool(std::cin>>value),std::string("Missing ")+field);}
void finite(double value,const char* field){require(std::isfinite(value),std::string("Invalid ")+field);}

int main(int argc,char** argv){
 try{
  require(argc==2,"Usage: flight-runner 3|6");
  const std::string mode=argv[1];require(mode=="3"||mode=="6","Invalid DoF");
  std::string protocol;read(protocol,"protocol");require(protocol=="STARLAB_FREE_FLIGHT_V1"||protocol=="STARLAB_FREE_FLIGHT_V2","Unsupported flight input protocol");
  Config cfg{};ProjectHost host;Phase phase{};phase.name="free-flight";
  double pitch=0,heading=0,motorAge=0;
  read(cfg.dt,"dt");read(cfg.site,"site");read(cfg.startUp,"start altitude");
  read(cfg.speed,"speed");read(pitch,"pitch");read(heading,"heading");
  read(host.windNorth,"north wind");read(host.windEast,"east wind");read(host.windDown,"down wind");
  read(phase.duration,"duration");read(motorAge,"motor age at start");read(phase.burn,"burn duration");
  read(phase.m0,"initial mass");read(phase.m1,"final mass");read(phase.cg,"CG");
  read(phase.I0.x,"Ix0");read(phase.I0.y,"Iy0");read(phase.I0.z,"Iz0");
  read(phase.I1.x,"Ix1");read(phase.I1.y,"Iy1");read(phase.I1.z,"Iz1");
  read(host.design.diameter,"reference diameter");read(host.exitArea,"motor exit area");
  read(host.design.baseOwner,"base owner");if(host.design.baseOwner=="-")host.design.baseOwner.clear();
  if(protocol=="STARLAB_FREE_FLIGHT_V2"){
   int custom=0;read(custom,"custom initial state");require(custom==0||custom==1,"Invalid initial state mode");
   read(cfg.roll,"initial roll");read(cfg.velocity.x,"initial axial velocity");read(cfg.velocity.y,"initial lateral velocity");read(cfg.velocity.z,"initial vertical velocity");
   read(cfg.rates.x,"initial roll rate");read(cfg.rates.y,"initial pitch rate");read(cfg.rates.z,"initial yaw rate");
   for(double v:{cfg.roll,cfg.velocity.x,cfg.velocity.y,cfg.velocity.z,cfg.rates.x,cfg.rates.y,cfg.rates.z})finite(v,"initial state");
   cfg.customInitial=custom==1;cfg.roll*=pi/180;
   if(cfg.customInitial)require(length(cfg.velocity)>0,"Initial velocity must be positive");
   int weatherCount=0;read(weatherCount,"wind point count");require(weatherCount==0||(weatherCount>=2&&weatherCount<=1000),"Invalid wind curve size");
   for(int i=0;i<weatherCount;++i){WindPoint point{};read(point.time,"wind time");read(point.wind.x,"north wind");read(point.wind.y,"east wind");read(point.wind.z,"down wind");
    for(double value:{point.time,point.wind.x,point.wind.y,point.wind.z})finite(value,"wind curve");
    require(point.time>=0&&point.time<=phase.duration&&(phase.weather.empty()||point.time>phase.weather.back().time)&&length(point.wind)<=200,"Invalid wind curve");phase.weather.push_back(point);
   }
   require(phase.weather.empty()||(phase.weather.front().time==0&&phase.weather.back().time==phase.duration),"Wind curve must cover phase");
   std::string atmosphereModel;read(atmosphereModel,"atmosphere model");require(atmosphereModel=="standard"||atmosphereModel=="archive","Unknown atmosphere model");cfg.legacy=atmosphereModel=="archive";
  }
  int count=0;read(count,"motor point count");require(count>=2&&count<=10000,"Invalid motor curve size");
  for(int i=0;i<count;++i){MotorPoint p{};read(p.time,"motor time");read(p.force,"motor thrust");finite(p.time,"motor time");finite(p.force,"motor thrust");phase.motor.push_back(p);}
  read(count,"part count");require(count>=1&&count<=64,"Invalid part count");
  for(int i=0;i<count;++i){
   geometry::Part part{};int kind=0;
   read(part.id,"part ID");read(kind,"part kind");
   require(kind>=0&&kind<=4,"Invalid part kind");part.kind=static_cast<geometry::Kind>(kind);
   read(part.station,"station");read(part.length,"length");read(part.foreRadius,"fore radius");read(part.aftRadius,"aft radius");
   read(part.number,"fin count");read(part.span,"span");read(part.root,"root chord");read(part.tip,"tip chord");read(part.sweep,"sweep");read(part.thickness,"fin thickness");
   host.design.parts.push_back(part);
  }
  require(cfg.dt>0&&cfg.dt<=1&&cfg.site>=0&&cfg.startUp>0&&cfg.speed>=0&&phase.duration>0&&phase.duration<=300
   &&phase.burn>0&&phase.burn<=phase.duration&&phase.m0>=phase.m1&&phase.m1>0,"Invalid flight bounds");
  require(phase.motor.front().time==0&&phase.motor.back().time==phase.burn,"Motor curve must cover burn");
  require(motorAge>=0&&motorAge<phase.burn,"Ray çıkışında motor hâlâ yanıyor olmalı.");
  const double originalBurn=phase.burn;
  std::vector<MotorPoint> remaining{{0,sampleCurve(phase.motor,motorAge,&MotorPoint::force)}};
  for(const auto& p:phase.motor)if(p.time>motorAge)remaining.push_back({p.time-motorAge,p.force});
  phase.motor=remaining;phase.burn=originalBurn-motorAge;
  require(host.exitArea>=0&&std::isfinite(host.exitArea),"Invalid motor exit area");
  cfg.theta=(90-pitch)*pi/180;cfg.yaw=heading*pi/180;cfg.wind={host.windNorth,host.windEast,host.windDown};
  cfg.host=&host;cfg.phases={phase};host.phase=&cfg.phases[0];host.site=cfg.site;host.legacy=cfg.legacy;
  std::ostringstream history,components;
  simulate(cfg,mode=="3",&history,&components);
  std::cout<<"STHISTORY\n"<<history.str()<<"STCOMPONENTS\n"<<components.str();
  return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
