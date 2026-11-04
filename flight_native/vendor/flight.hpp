#pragma once
#include "geometry.hpp"
namespace flight {
struct Air{double T,P,rho,a,mu;};
Air atmosphere(double h,bool legacy){
 double T,P,a;const double H=6356766*h/(6356766+h);
 require(h>=0&&H<=20000,"Outside configured atmospheric altitude range");
 if(legacy){
  double C=h<11000?15.04-.00649*h:-56.4;
  P=1000*(h<11000?101.29*std::pow((C+273.1)/288.08,5.256):22.65*std::pow(2.718,1.73-.000157*h));
  T=C+273.1;a=331+.61*C;
 }else{
  T=288.15-.0065*std::min(H,11000.);
  P=101325*std::pow(T/288.15,gravity/(287.05*.0065))*std::exp(-gravity*std::max(0.,H-11000)/(287.05*T));
  a=std::sqrt(1.4*287.05*T);
 }
 return {T,P,P/((legacy?286.9:287.05)*T),a,1.716e-5*std::pow(T/273.15,1.5)*383.55/(T+110.4)};
}
// Coefficients belong to the whole named component or symmetric fin set,
// resolved on vehicle wind/body axes, referenced to its own area/length.
enum Col {CDoff,CDon,CDa2,CLa,CLq,CYb,Cl0,Clp,Cma,Cmq,Cnb,Cnr,Count};
using Coeff=std::array<double,Count>;
struct Row{double mach;Coeff c;};
struct Surface{std::string name;double area,d,limit; Vec ref;std::vector<Row> table;};
Coeff interpolate(const Surface& s,double mach){
 const auto& t=s.table;require(mach>=t.front().mach&&mach<=t.back().mach,"Mach outside table: "+s.name);
 auto hi=std::lower_bound(t.begin(),t.end(),mach,[](const Row& a,double b){return a.mach<b;});
 if(hi==t.begin()||hi->mach==mach)return hi->c;
 auto lo=hi-1;double f=(mach-lo->mach)/(hi->mach-lo->mach);Coeff c{};
 for(int j=0;j<Count;++j)c[j]=lo->c[j]+f*(hi->c[j]-lo->c[j]);return c;
}
struct MotorPoint{double time,force;};
struct WindPoint{double time;Vec wind;};
struct Phase{std::string name; double duration,burn,thrust,m0,m1,cg;Vec I0,I1;std::vector<Surface> surfaces;std::vector<MotorPoint> motor;std::vector<WindPoint> weather;};
template<class Point,class Value>
Value sampleCurve(const std::vector<Point>& curve,double t,Value Point::*field){
 auto hi=std::lower_bound(curve.begin(),curve.end(),t,[](const Point& a,double b){return a.time<b;});
 if(hi==curve.begin())return (*hi).*field;
 if(hi==curve.end())return curve.back().*field;
 auto lo=hi-1;double f=(t-lo->time)/(hi->time-lo->time);
 return (1-f)*((*lo).*field)+f*((*hi).*field);
}
struct Config{double dt,site,startUp,speed,theta;Vec wind;bool legacy;std::vector<Phase> phases;double roll=0,yaw=0;Vec rates{},velocity{};bool customInitial=false;const geometry::Host* host=nullptr;double phaseStart=0;};
struct State{Vec pos,vel,omega;Quat attitude;};
State operator+(State a,State b){return {a.pos+b.pos,a.vel+b.vel,a.omega+b.omega,a.attitude+b.attitude};}
State operator*(double k,State a){return {k*a.pos,k*a.vel,k*a.omega,k*a.attitude};}
struct Load{Vec f,m;};
struct Forces{Vec thrust,weight,net;};
Forces combineForces(const Phase& ph,double age,bool firing,double mass,Quat k,Vec aero){
 double thrust=firing?(ph.motor.empty()?ph.thrust:sampleCurve(ph.motor,age,&MotorPoint::force)):0;
 Forces b{};b.thrust={thrust,0,0};b.weight=inverseRotate(k,{0,0,mass*gravity});
 b.net=aero+b.thrust+b.weight;return b;
}
struct Sample{State rate; Air air; double V,alpha,beta,Q,mach,mass;Vec I;Load total;Forces forces;std::vector<Load> parts;std::vector<std::string> names;std::vector<double> reynolds;std::vector<geometry::Component> components;double rocketCp=0,rocketCD=0,rocketCL=0,rocketCm=0;bool cpDefined=false;Vec gravityN{0,0,gravity},motorMoment{};};
// Check Newton's law independently in Earth axes. Indicate the phase and time on failure.
void auditForces(const Sample& o,const State& s,Quat k,const Phase& ph,double age){
 const Vec expected=rotate(k,o.total.f+o.forces.thrust)+o.mass*o.gravityN;
 const Vec fromRate=rotate(k,o.mass*(o.rate.vel+cross(s.omega,s.vel)));
 double tolerance=1e-8+1e-9*(length(o.total.f)+length(o.forces.thrust)+o.mass*length(o.gravityN));
 if(!(std::isfinite(tolerance)&&length(rotate(k,o.forces.net)-expected)<=tolerance&&length(fromRate-expected)<=tolerance)){
  throw std::runtime_error("Force balance [N], phase="+ph.name+" local_s="+std::to_string(age));
 }
}
Sample evaluate(const Config& cfg,const Phase& ph,const State& s,double age,bool firing,bool planar){
 Sample o{};Quat k=unit(s.attitude);
 geometry::Request req{cfg.phaseStart+age,age,ph.name,s.pos,k,s.vel,s.omega,firing};
 geometry::Conditions supplied{};geometry::Result geometric{};
 Vec wind;double suppliedMass=0;
 if(cfg.host){
  supplied=cfg.host->conditions(req);const auto& design=cfg.host->activeDesign(req);
  require(length(supplied.groundVelocityB-s.vel)<1e-9,"Host velocity mismatch");
  require(std::abs(supplied.cgStation+ph.cg)<1e-9,"Standalone DoF requires matching fixed phase CG");
  // The solver owns the side of a discontinuity for endpoint RK4 evaluations.
  require(firing==supplied.firing,"Host motor state disagrees with active RK interval");
  geometric=geometry::evaluate(design,supplied);
  wind=rotate(k,supplied.windB);suppliedMass=supplied.mass;
  o.air={supplied.temperature,supplied.pressure,supplied.rho,supplied.soundSpeed,supplied.mu};
  o.gravityN=rotate(k,supplied.gravityB);o.motorMoment=supplied.motorMomentCG;
  o.components=geometric.parts;o.rocketCp=geometric.cp;o.cpDefined=geometric.cpDefined;
  o.rocketCD=geometric.cd;o.rocketCL=geometric.cl;o.rocketCm=geometric.cm;
  for(const auto& part:geometric.parts){o.names.push_back(part.id);o.parts.push_back({part.force,part.moment});}
  for(const auto& part:design.parts)o.reynolds.push_back(supplied.rho*length(s.vel-supplied.windB)*part.length/supplied.mu);
  o.total={geometric.aerodynamic,geometric.momentCG};
 }else{
  wind=ph.weather.empty()?cfg.wind:sampleCurve(ph.weather,age,&WindPoint::wind);
  o.air=atmosphere(cfg.site-s.pos.z,cfg.legacy);
 }
 Vec rel=s.vel-inverseRotate(k,wind);o.V=length(rel);
 o.mach=o.V/o.air.a;o.Q=.5*o.air.rho*o.V*o.V;
 o.alpha=o.V?std::atan2(rel.z,rel.x):0;o.beta=o.V?std::atan2(rel.y,std::hypot(rel.x,rel.z)):0;
 double ca=std::cos(o.alpha),sa=std::sin(o.alpha),cb=std::cos(o.beta),sb=std::sin(o.beta);
 Vec eD={ca*cb,sb,sa*cb},eY={-ca*sb,cb,-sa*sb},eL={sa,0,-ca};
 if(!cfg.host)for(const Surface& sf:ph.surfaces){
  o.names.push_back(sf.name);o.reynolds.push_back(o.air.rho*o.V*sf.d/o.air.mu);
  Load l{};
  if(o.V>0){
   require(std::atan2(std::hypot(rel.y,rel.z),rel.x)<=sf.limit,"Incidence outside envelope: "+sf.name);
   Coeff c=interpolate(sf,o.mach);double qs=o.Q*sf.area;
   // Q*d/(2V) evaluated as rho*V*d/4 to avoid a singular rate term.
   double rateScale=.25*o.air.rho*o.V*sf.area*sf.d;
   double drag=qs*(c[firing?CDon:CDoff]+c[CDa2]*(o.alpha*o.alpha+o.beta*o.beta));
   require(drag>=0,"Negative component drag");
   l.f=(-drag)*eD+(qs*c[CYb]*o.beta)*eY+(qs*c[CLa]*o.alpha+rateScale*c[CLq]*s.omega.y)*eL;
   Vec freeMoment=sf.d*Vec{qs*c[Cl0]+rateScale*c[Clp]*s.omega.x,qs*c[Cma]*o.alpha+rateScale*c[Cmq]*s.omega.y,
                                      qs*c[Cnb]*o.beta+rateScale*c[Cnr]*s.omega.z};
   l.m=freeMoment+cross(sf.ref-Vec{ph.cg,0,0},l.f);
  }
  o.parts.push_back(l);o.total.f=o.total.f+l.f;o.total.m=o.total.m+l.m;
 }
 double f=ph.burn?std::max(0.,std::min(1.,age/ph.burn)):1;
 o.mass=ph.m0+f*(ph.m1-ph.m0);o.I=ph.I0+f*(ph.I1-ph.I0);
 if(cfg.host){o.mass=suppliedMass;o.forces={supplied.thrustB,geometric.weight,geometric.net};}
 else o.forces=combineForces(ph,age,firing,o.mass,k,o.total.f);
 Vec Idot=firing?(1/ph.burn)*(ph.I1-ph.I0):Vec{};
 Vec H={o.I.x*s.omega.x,o.I.y*s.omega.y,o.I.z*s.omega.z};
 Vec changing={Idot.x*s.omega.x,Idot.y*s.omega.y,Idot.z*s.omega.z};
 Vec torque=o.total.m+o.motorMoment-cross(s.omega,H)-changing;
 o.rate.omega={torque.x/o.I.x,torque.y/o.I.y,torque.z/o.I.z};
 o.rate.vel=(1/o.mass)*o.forces.net-cross(s.omega,s.vel);
 o.rate.pos=rotate(k,s.vel);o.rate.attitude=.5*product(s.attitude,{0,s.omega});
 if(planar){
  require(std::abs(wind.y)+std::abs(s.vel.y)+std::abs(s.omega.x)+std::abs(s.omega.z)+std::abs(k.v.x)+std::abs(k.v.z)<1e-10,"State/wind outside 3DOF plane");
  require(std::abs(o.forces.net.y)+std::abs(o.total.m.x+o.motorMoment.x)+std::abs(o.total.m.z+o.motorMoment.z)<1e-9,"Loads outside 3DOF plane");
  double st=2*k.s*k.v.y,ct=k.s*k.s-k.v.y*k.v.y,q=s.omega.y;
  o.rate.pos={ct*s.vel.x+st*s.vel.z,0,-st*s.vel.x+ct*s.vel.z};
  o.rate.vel={o.forces.net.x/o.mass-q*s.vel.z,0,o.forces.net.z/o.mass+q*s.vel.x};
  o.rate.omega={0,(o.total.m.y+o.motorMoment.y-Idot.y*q)/o.I.y,0};
 }
 auditForces(o,s,k,ph,age);
 Vec angularRate={o.I.x*o.rate.omega.x,o.I.y*o.rate.omega.y,o.I.z*o.rate.omega.z};
 double momentError=length(angularRate+changing+cross(s.omega,H)-o.total.m-o.motorMoment);
 double momentScale=length(o.total.m)+length(o.motorMoment)+length(changing)+length(cross(s.omega,H));
 require(std::isfinite(momentScale)&&momentError<=1e-9+1e-9*momentScale,
         "Moment balance [Nm], phase="+ph.name+" local_s="+std::to_string(age));
 return o;
}
State step(const Config& c,const Phase& p,State s,double age,double h,bool burn,bool plane){
 auto rhs=[&](State x,double t){return evaluate(c,p,x,t,burn,plane).rate;};
 State a=rhs(s,age),b=rhs(s+(.5*h)*a,age+.5*h),d=rhs(s+(.5*h)*b,age+.5*h),e=rhs(s+h*d,age+h);
 s=s+(h/6)*(a+2*b+2*d+e);s.attitude=unit(s.attitude);
 require(std::isfinite(length(s.pos)+length(s.vel)+length(s.omega)),"Non-finite state");return s;
}
// Full step versus two half steps. If they disagree, reject and subdivide.
State checkedStep(const Config& c,const Phase& p,State s,double age,double h,bool burn,bool plane,int depth=0){
 State full=step(c,p,s,age,h,burn,plane);
 State half=step(c,p,s,age,h/2,burn,plane);
 State fine=step(c,p,half,age+h/2,h/2,burn,plane);
 Quat a=full.attitude,b=fine.attitude;
 if(a.s*b.s+dot(a.v,b.v)<0)b=(-1)*b;
 double chord=std::hypot(a.s-b.s,length(a.v-b.v));
 if(length(full.pos-fine.pos)<=1e-4&&length(full.vel-fine.vel)<=1e-4&&
    length(full.omega-fine.omega)<=1e-6&&chord<=5e-7)return fine;
 require(depth<12,"RK4 convergence limit: phase="+p.name+" local_s="+std::to_string(age));
 half=checkedStep(c,p,s,age,h/2,burn,plane,depth+1);
 return checkedStep(c,p,half,age+h/2,h/2,burn,plane,depth+1);
}
Config readConfig(const std::string& file){
 std::ifstream in(file);require(bool(in),"Cannot read "+file);Config c{};std::string line;int n=0;bool setup=false;
 while(std::getline(in,line)){++n;std::istringstream r(line.substr(0,line.find('#')));std::string key;if(!(r>>key))continue;
  if(key=="setup"){require(!setup&&c.phases.empty(),"Duplicate/late setup");std::string model;r>>c.dt>>c.site>>c.startUp>>c.speed>>c.theta>>c.wind.x>>c.wind.y>>c.wind.z>>model;
   require(model=="standard"||model=="archive","Unknown atmosphere");c.legacy=model=="archive";setup=true;c.theta*=pi/180;
  }else if(key=="initial"){require(setup&&c.phases.empty()&&!c.customInitial,"Initial must follow setup once");c.customInitial=true;
   r>>c.roll>>c.yaw>>c.velocity.x>>c.velocity.y>>c.velocity.z>>c.rates.x>>c.rates.y>>c.rates.z;
   c.roll*=pi/180;c.yaw*=pi/180;
  }else if(key=="phase"){Phase p{};r>>p.name>>p.duration>>p.burn>>p.thrust>>p.m0>>p.m1>>p.cg>>p.I0.x>>p.I0.y>>p.I0.z>>p.I1.x>>p.I1.y>>p.I1.z;
   require(p.duration>0&&p.burn>=0&&p.burn<=p.duration&&p.thrust>=0&&p.m0>=p.m1&&p.m1>0,"Invalid phase");
   for(Vec I:{p.I0,p.I1})require(I.x>0&&I.y>0&&I.z>0&&I.x<=I.y+I.z&&I.y<=I.x+I.z&&I.z<=I.x+I.y,"Invalid rigid body inertia");
   require(p.burn>0||(p.m0==p.m1&&length(p.I0-p.I1)==0&&p.thrust==0),"Mass/thrust change without burn");c.phases.push_back(p);
  }else if(key=="motor"){require(!c.phases.empty(),"Motor needs phase");auto& p=c.phases.back();MotorPoint v{};r>>v.time>>v.force;
   require(v.force>=0&&v.time>=0&&v.time<=p.burn&&(p.motor.empty()||v.time>p.motor.back().time),"Invalid motor curve");p.motor.push_back(v);
  }else if(key=="wind"){require(!c.phases.empty(),"Wind needs phase");auto& p=c.phases.back();WindPoint w{};r>>w.time>>w.wind.x>>w.wind.y>>w.wind.z;
   require(w.time>=0&&w.time<=p.duration&&(p.weather.empty()||w.time>p.weather.back().time),"Invalid wind curve");p.weather.push_back(w);
  }else if(key=="surface"){require(!c.phases.empty(),"Surface needs phase");Surface s{};r>>s.name>>s.area>>s.d>>s.ref.x>>s.ref.y>>s.ref.z>>s.limit;
   require(s.area>0&&s.d>0&&s.limit>0&&s.limit<=90,"Invalid surface");s.limit*=pi/180;c.phases.back().surfaces.push_back(s);
  }else if(key=="row"){require(!c.phases.empty()&&!c.phases.back().surfaces.empty(),"Row needs surface");auto& t=c.phases.back().surfaces.back().table;Row a{};r>>a.mach;
   for(double& x:a.c)r>>x;require(a.mach>=0&&(t.empty()||a.mach>t.back().mach),"Mach nodes must increase");t.push_back(a);
  }else throw std::runtime_error("Unknown key: "+key);
  require(bool(r),"Missing field at line "+std::to_string(n));std::string extra;require(!(r>>extra),"Extra field at line "+std::to_string(n));
 }
 require(setup&&c.dt>0&&c.site>=0&&c.startUp>=0&&c.speed>=0&&!c.phases.empty(),"Invalid setup");
 for(auto& p:c.phases){require(!p.surfaces.empty(),"Missing surfaces");for(auto& s:p.surfaces)require(s.table.size()>=2,"Need two Mach nodes");
  require(p.motor.empty()||(p.motor.size()>1&&p.motor.front().time==0&&p.motor.back().time==p.burn),"Motor curve must cover burn");
  require(p.weather.empty()||(p.weather.front().time==0&&p.weather.back().time==p.duration),"Wind curve must cover phase");
 }
 return c;
}
State initial(const Config& c){
 Quat pitch{std::cos(c.theta/2),{0,std::sin(c.theta/2),0}};
 Quat roll{std::cos(c.roll/2),{std::sin(c.roll/2),0,0}};
 Quat yaw{std::cos(c.yaw/2),{0,0,std::sin(c.yaw/2)}};
 return {{0,0,-c.startUp},c.customInitial?c.velocity:Vec{c.speed,0,0},c.rates,product(yaw,product(pitch,roll))};
}
State simulate(const Config& input,bool plane,std::ostream* history,std::ostream* parts){
 Config c=input;State s=initial(c);double t=0;if(history)*history<<std::setprecision(14)<<"t_s,phase,firing,n_m,e_m,down_m,u,v,w,p,q,r,k0,k1,k2,k3,T_K,P_Pa,rho,a,V_air_mps,alpha,beta,Mach,Q_Pa,mass,Ix,Iy,Iz,Fx,Fy,Fz,Mx,My,Mz,Tx_N,Ty_N,Tz_N,Wx_N,Wy_N,Wz_N,FnetX_N,FnetY_N,FnetZ_N"<<(c.host?",regime,CP_m,CD,CL,CmCG":"")<<'\n';
 if(parts)*parts<<std::setprecision(14)<<"t_s,phase,surface,Re,Fx,Fy,Fz,MxCG,MyCG,MzCG"<<(c.host?",CP_m,CD,CL,CmCG,tail_turn_deg,tail_inlet_method":"")<<'\n';
 for(size_t i=0;i<c.phases.size();++i){const Phase& p=c.phases[i];c.phaseStart=t;if(i){
  Vec delta={p.cg-c.phases[i-1].cg,0,0};s.pos=s.pos+rotate(s.attitude,delta);s.vel=s.vel+cross(s.omega,delta);
 }
 double age=0;while(true){
  bool firing=age<p.burn;Sample a=evaluate(c,p,s,age,firing,plane);
  if(history){*history<<t+age<<','<<p.name<<','<<firing;
   for(double x:{s.pos.x,s.pos.y,s.pos.z,s.vel.x,s.vel.y,s.vel.z,s.omega.x,s.omega.y,s.omega.z,s.attitude.s,s.attitude.v.x,s.attitude.v.y,s.attitude.v.z,a.air.T,a.air.P,a.air.rho,a.air.a,a.V,a.alpha,a.beta,a.mach,a.Q,a.mass,a.I.x,a.I.y,a.I.z,a.total.f.x,a.total.f.y,a.total.f.z,a.total.m.x,a.total.m.y,a.total.m.z,a.forces.thrust.x,a.forces.thrust.y,a.forces.thrust.z,a.forces.weight.x,a.forces.weight.y,a.forces.weight.z,a.forces.net.x,a.forces.net.y,a.forces.net.z})*history<<','<<x;
   if(c.host){*history<<','<<geometry::regime(a.mach)<<',';if(a.cpDefined)*history<<a.rocketCp;*history<<','<<a.rocketCD<<','<<a.rocketCL<<','<<a.rocketCm;}*history<<'\n';}
  if(parts)for(size_t j=0;j<a.parts.size();++j){auto l=a.parts[j];*parts<<t+age<<','<<p.name<<','<<a.names[j]<<','<<a.reynolds[j];
   for(double x:{l.f.x,l.f.y,l.f.z,l.m.x,l.m.y,l.m.z})*parts<<','<<x;
   if(c.host){const auto& g=a.components[j];*parts<<',';if(g.cpDefined)*parts<<g.cp;*parts<<','<<g.cd<<','<<g.cl<<','<<g.cm<<',';if(!g.inletMethod.empty())*parts<<g.turnRad*180/pi;*parts<<','<<g.inletMethod;}
   *parts<<'\n';}
  if(age==p.duration)break;
  double next=std::min(age+c.dt,firing?p.burn:p.duration);
  for(const auto& node:p.motor)if(node.time>age)next=std::min(next,node.time);
  for(const auto& node:p.weather)if(node.time>age)next=std::min(next,node.time);
  require(next>age,"Step too small");s=checkedStep(c,p,s,age,next-age,firing,plane);age=next;
  require(s.pos.z<=0,"Reached ground: end free-flight scenario before impact");
 }t+=p.duration;
 }return s;
}
} // namespace flight

