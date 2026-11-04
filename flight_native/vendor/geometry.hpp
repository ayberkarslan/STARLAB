#pragma once
#include "flight_types.hpp"
namespace flight { namespace geometry {
// Small-incidence preliminary model. Design stations increase aft.
constexpr double subLimit=.8,superStart=1.3,maxMach=3.;
inline const char* regime(double m){return m<=subLimit?"subsonic":m<superStart?"transonic_approx":"supersonic";}
inline double blend(double x,double a,double b){double w=x*x*(3-2*x);return a+(b-a)*w;}
template<class Sub,class Super>
inline double acrossMach(double m,Sub sub,Super super){
 if(m<=subLimit)return sub(m);
 if(m>=superStart)return super(m);
 return blend((m-subLimit)/(superStart-subLimit),sub(subLimit),super(superStart));
}
inline double baseDrag(double m){
 return acrossMach(m,[](double x){return .12+.13*x*x;},[](double x){return .25/x;});
}
inline double conePressure(double m,double sine){
 double low=.8*sine*sine;
 auto sup=[&](double x){return 2.1*sine*sine+.5*sine/std::sqrt(x*x-1);};
 if(m<=subLimit)return low;
 if(m<=1)return blend((m-subLimit)/(1-subLimit),low,sine);
 if(m<superStart)return blend((m-1)/(superStart-1),sine,sup(superStart));
 return sup(m);
}
// Stagnation pressure over freestream dynamic pressure; normal shock for M>1.
inline double stagnationCD(double m){
 if(m<1e-4)return 1.;
 double m2=m*m;
 double ratio=m<=1?std::pow(1+.2*m2,3.5):
      std::pow(1.2*m2,3.5)*std::pow(2.4/(2.8*m2-.4),2.5);
 return (ratio-1)/(.7*m2);
}
inline double prandtlMeyer(double m){
 double b=std::sqrt(m*m-1);
 return std::sqrt(6.)*std::atan(b/std::sqrt(6.))-std::atan(b);
}
// Attached planar expansion proxy for a slender conical boattail (gamma=1.4).
inline double expansionDeficit(double m,double turn){
 if(turn==0)return 0;
 double target=prandtlMeyer(m)+turn,lo=m,hi=2*m;
 require(target<(std::sqrt(6.)-1)*pi/2,"Expansion exceeds Prandtl-Meyer limit");
 while(prandtlMeyer(hi)<target)hi*=2;
 for(int i=0;i<42;++i){
  double mid=.5*(lo+hi);
  if(prandtlMeyer(mid)<target)lo=mid;else hi=mid;
 }
 double pRatio=std::pow((1+.2*m*m)/(1+.2*hi*hi),3.5);
 return (1-pRatio)/(.7*m*m);
}
enum class Kind { Cone, Tube, Transition, Boattail, Fins };
struct Part {
 std::string id; Kind kind; double station,length,foreRadius,aftRadius;
 // For an equally spaced set of trapezoidal, uncanted fins, foreRadius is mount radius.
 int number=0; double span=0,root=0,tip=0,sweep=0,thickness=0;
};
struct Design {
 double diameter; std::vector<Part> parts;
 std::string baseOwner; // Sole owner of the exposed rear disk; empty for no exposed disk.
};
struct TailInlet {std::string partId;double mach,pressurePa;};
struct Conditions {
 double rho,soundSpeed,mu,mass,cgStation;
 Vec groundVelocityB,windB,gravityB,thrustB,motorMomentCG;
 bool firing; double motorExitArea;double temperature=288.15,pressure=101325;
 std::vector<TailInlet> tailInlets;
};
struct Request {double globalTime,phaseTime;std::string stage;Vec position;Quat orientation;Vec velocityB,omegaB;bool firing=false;};
struct Host {
 virtual const Design& activeDesign(const Request&) const=0;
 virtual Conditions conditions(const Request&) const=0;
 virtual ~Host()=default;
};
struct Component {
 std::string id;bool cpDefined=false;double cp=0,cnSlope=0,cd=0,cl=0,cm=0;
 Vec force{},moment{};
 double turnRad=0;std::string inletMethod;
};
struct Result {
 std::vector<Component> parts;bool cpDefined=false;double cp=0,cd=0,cl=0,cm=0;
 Vec aerodynamic{},momentCG{},weight{},net{},netMoment{};
};
inline double disk(double r){return pi*r*r;}
inline Result evaluate(const Design& design,const Conditions& in){
 require(std::isfinite(design.diameter)&&design.diameter>0&&!design.parts.empty(),"Missing active geometry");
 require(in.rho>0&&in.mu>0&&in.soundSpeed>0&&in.mass>0&&in.motorExitArea>=0,"Invalid environment or mass");
 Vec v=in.groundVelocityB-in.windB;double V=length(v),mach=V/in.soundSpeed;
 double incidence=std::atan2(std::hypot(v.y,v.z),v.x);
 require(mach<=maxMach&&incidence<=10*pi/180,"Geometry envelope: Mach <= 3.0 and incidence <= 10 deg");
 double S=disk(design.diameter/2),Q=.5*in.rho*V*V,baseCD=baseDrag(mach);
 Vec flow=V?(1/V)*v:Vec{1,0,0};
 double alpha=std::atan2(v.z,v.x); Vec liftAxis={std::sin(alpha),0,-std::cos(alpha)};
 Result out; double slopeSum=0,absSlope=0,weighted=0;int baseCount=0;
 for(const Part& p:design.parts){
  require(!p.id.empty()&&std::none_of(out.parts.begin(),out.parts.end(),[&](const Component& c){return c.id==p.id;}),"Missing or duplicate component ID");
  require(std::isfinite(p.station)&&p.length>0&&p.foreRadius>=0&&p.aftRadius>=0,"Invalid geometry: "+p.id);
  Component c{};c.id=p.id;double wetted=0,dragArea=0,localCp=0,frictionLength=p.length;
  if(p.kind==Kind::Fins){
   require(p.number>=3&&p.span>0&&p.root>0&&p.tip>=0&&p.thickness>=0&&p.foreRadius>0,"Invalid fin geometry");
   double chords=p.root+p.tip;
   double midChordLine=std::hypot(p.span,p.sweep+.5*(p.tip-p.root));
   double area=.5*p.span*chords,interference=1+p.foreRadius/(p.foreRadius+p.span);
   c.cnSlope=acrossMach(mach,[&](double m){
    return interference*4*p.number*std::pow(p.span/design.diameter,2)/
            (1+std::sqrt(1+(1-m*m)*std::pow(2*midChordLine/chords,2)));
   },[&](double m){
    // Two-sided linear supersonic flat plate slope; equally spaced set projects by N/2.
    return interference*.5*p.number*area/S*4/std::sqrt(m*m-1);
   });
   localCp=p.sweep*(p.root+2*p.tip)/(3*chords)+(chords-p.root*p.tip/chords)/6;
   frictionLength=(2./3)*(p.root*p.root+p.root*p.tip+p.tip*p.tip)/chords;
   if(mach>.5){
    double ar=2*p.span*p.span/area;
    auto chordFraction=[&](double m){
     double k=ar*std::sqrt(m*m-1);
     require(k>1,"Supersonic CP correlation outside supported fin aspect ratio");
     return (k-.67)/(2*k-1);
    };
    double fraction=mach<2?blend((mach-.5)/1.5,.25,chordFraction(2)):chordFraction(mach);
    localCp+=(fraction-.25)*frictionLength;
   }
   wetted=p.number*p.span*chords*(1+2*p.thickness/frictionLength);
   dragArea=p.number*p.thickness*p.span*(stagnationCD(mach)/(1+std::pow(p.sweep/p.span,2))+baseCD);
  }else{
   double r0=p.foreRadius,r1=p.aftRadius,dr=r1-r0;
   require(r0+r1>0,"Zero diameter component");
   require((p.kind!=Kind::Cone||(r0==0&&r1>0))&&(p.kind!=Kind::Tube||dr==0)
        &&(p.kind!=Kind::Transition||dr>0)&&(p.kind!=Kind::Boattail||dr<0),"Part type/radii mismatch");
   c.cnSlope=2*(disk(r1)-disk(r0))/S;
   localCp=p.length*(r0+2*r1)/(3*(r0+r1));
   wetted=pi*(r0+r1)*std::hypot(dr,p.length);
   if(dr>0)dragArea=conePressure(mach,dr/std::hypot(dr,p.length))*(disk(r1)-disk(r0));
   if(dr<0){
    double gamma=p.length/(-2*dr);
    const Part* upstream=nullptr;
    for(const Part& u:design.parts){
     if(u.kind==Kind::Fins||&u==&p||std::abs(u.station+u.length-p.station)>1e-7||
        std::abs(u.aftRadius-r0)>1e-7)continue;
     require(!upstream,"Ambiguous upstream contour: "+p.id);upstream=&u;
    }
    double incoming=upstream?std::atan2(upstream->aftRadius-upstream->foreRadius,upstream->length):0.;
    double turn=incoming-std::atan2(dr,p.length);c.turnRad=turn;
    c.inletMethod=mach<=subLimit?"subsonic_empirical":upstream?"freestream_proxy":"axial_freestream_proxy";
    if(mach>subLimit)require(turn>=0&&turn<=20*pi/180,"Unsupported boattail compression or turn >20 deg: "+p.id);
    double deficit=acrossMach(mach,[&](double m){return (.12+.13*m*m)*std::max(0.,std::min(1.,(3-gamma)/2));},
                                [&](double m){return expansionDeficit(m,turn);});
    if(mach>1.2){
     const TailInlet* local=nullptr;
     for(const auto& v:in.tailInlets)if(v.partId==p.id){require(!local,"Duplicate boattail inlet");local=&v;}
     if(local){
      require(std::isfinite(local->mach)&&local->mach>1&&std::isfinite(local->pressurePa)&&local->pressurePa>0&&in.pressure>0,"Invalid local supersonic inlet");
      double localQ=.7*local->pressurePa*local->mach*local->mach;
      double wallP=local->pressurePa-localQ*expansionDeficit(local->mach,turn);
      double localDeficit=(in.pressure-wallP)/Q;
      deficit=blend(std::min(1.,(mach-1.2)/(superStart-1.2)),deficit,localDeficit);
      c.inletMethod=mach<superStart?"local_blend":"local_supersonic";
      if(!upstream)c.inletMethod+="_axial_proxy";
     }
    }
    dragArea=deficit*(disk(r0)-disk(r1));
   }
  }
  if(design.baseOwner==p.id){
   require(p.kind!=Kind::Fins,"Base disk cannot belong to fins");++baseCount;
   double base=disk(p.aftRadius),covered=in.firing?in.motorExitArea:0;
   require(covered<=base,"Motor exit area exceeds exposed disk");dragArea+=baseCD*(base-covered);
  }
  // Smooth plate average Cf, laminar/turbulent transition blended in log Re.
  double Re=in.rho*V*frictionLength/in.mu,cf=0;
  if(Re>0){double lam=1.328/std::sqrt(Re);
   if(Re<=3e5)cf=lam;
   else{double f=std::min(1.,std::log(Re/3e5)/std::log(2.));cf=(1-f)*lam+f*.455/(std::pow(std::log10(Re),2.58)*std::pow(1+.144*mach*mach,.65));}
  }
  double ca=(dragArea+wetted*cf)/S;
  // Body-axis normal load, small-angle (sin incidence) extension in pitch/yaw.
  Vec cForce={-ca,-c.cnSlope*flow.y,-c.cnSlope*flow.z};
  c.cpDefined=c.cnSlope!=0;
  if(c.cpDefined)c.cp=p.station+localCp;
  double arm=c.cpDefined?in.cgStation-c.cp:0;
  c.cd=-dot(cForce,flow);c.cl=dot(cForce,liftAxis);
  Vec cMoment=(1/design.diameter)*cross({arm,0,0},cForce);c.cm=cMoment.y;
  c.force=(Q*S)*cForce;c.moment=(Q*S*design.diameter)*cMoment;
  out.parts.push_back(c);out.aerodynamic=out.aerodynamic+c.force;out.momentCG=out.momentCG+c.moment;
  out.cd+=c.cd;out.cl+=c.cl;out.cm+=c.cm;
  slopeSum+=c.cnSlope;absSlope+=std::abs(c.cnSlope);weighted+=c.cnSlope*c.cp;
 }
 require(design.baseOwner.empty()||baseCount==1,"Missing or duplicated base owner");
 out.cpDefined=std::abs(slopeSum)>1e-12*std::max(1.,absSlope);
 if(out.cpDefined)out.cp=weighted/slopeSum;
 out.weight=in.mass*in.gravityB;out.net=out.aerodynamic+in.thrustB+out.weight;
 out.netMoment=out.momentCG+in.motorMomentCG;
 require(std::isfinite(length(out.net)+length(out.netMoment)+out.cd+out.cm+out.cl),"Non-finite component result");
 return out;
}
inline Result evaluateAt(const Host& host,const Request& request){
 return evaluate(host.activeDesign(request),host.conditions(request));
}
}} // flight::geometry

