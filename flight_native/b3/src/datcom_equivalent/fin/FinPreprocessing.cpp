#include "datcom_equivalent/fin/FinPreprocessing.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "datcom_equivalent/body/HybridPressureMachine.hpp"
#include "datcom_equivalent/common/Float32.hpp"
#include "datcom_equivalent/fin/FinSectionProgramData.hpp"
namespace datcom_equivalent::fin { namespace {
using common::f32; using body::HybridPressureMachine;
constexpr double kMPerInternal=0.3048779964447021484375, kRadPerDeg=57.295780181884766, kMissing=1.0000000031710769e-30;
std::uint32_t bits(double x){ return common::f32Bits(x); }
const std::array<std::uint32_t,50> kXBits={0,981668463,990057071,994352038,998445679,1000593163,1002740647,1006834287,1008981771,1017370378,1022739087,1025758986,1028443340,1031127694,1034147594,1036831948,1039516302,1041194024,1042536201,1043878378,1045220555,1046562732,1047904909,1048911543,1049582632,1050253721,1050924810,1051595899,1052266988,1052938077,1053609166,1054280255,1055286888,1056964609,1057803470,1058642331,1059481192,1060320053,1061158914,1061997775,1062333319,1062668863,1063004407,1063339951,1063675495,1064011039,1064346583,1064682127,1065017671,1065353216};
double fromBits(std::uint32_t v){float x;std::memcpy(&x,&v,4);return x;}
const HybridPressureMachine::OperationLedger& ledger(){static const HybridPressureMachine::OperationLedger l{section_program::kFunctions,section_program::kInitialDwords,section_program::kTableDwords,section_program::kOperations,section_program::kThunks,section_program::kMathIntrinsics};return l;}
double interp(double a,double b,double t){return f32(f32(a)+t*f32(f32(b)-f32(a)));}
double profileInterp(const std::array<double,50>& x,const std::array<double,50>& y,double q){for(int i=1;i<50;++i)if(q<=x[i])return f32(y[i]-(x[i]-q)*(y[i]-y[i-1])/(x[i]-x[i-1]));return y[49];}
double effectiveLer(const FinGeometry& g){
 const double y0=f32(g.sspanM[0]/kMPerInternal), y1=f32(g.sspanM[1]/kMPerInternal), c0=f32(g.chordM[0]/kMPerInternal), c1=f32(g.chordM[1]/kMPerInternal);
 const double area=f32((c0+c1)*f32(y1-y0)/2.0), moment=(y1-y0)/6.0*(y0*(2*c0+c1)+y1*(c0+2*c1)), q=f32(moment/area);
 const double l0=g.lerM.size()>0&&g.lerM[0]?f32(*g.lerM[0]/kMPerInternal):0, l1=g.lerM.size()>1&&g.lerM[1]?f32(*g.lerM[1]/kMPerInternal):0;
 return f32(l1-(y1-q)*(l1-l0)/(y1-y0));
}
std::array<double,7> kernel(double mach,double re,double ler,double thick,double nonlinear,double chord,const std::array<double,50>& profile,const std::array<double,50>& x){
 const std::uint32_t title=0x006391E0,count=0x0063897C,machs=0x00638980, reyn=0x00638A20, slot=0x0058224C, radius=0x00636B80, thickness=0x00636B84, c=0x00635C5C,n=0x00635C60, xa=0x006384A0,pa=0x00638630,ma=0x00638568;
 const std::array<std::uint32_t,7> out={0x00636B8C,0x00636BDC,0x00636C2C,0x00636FAC,0x00636FB0,0x00636FB4,0x00636FB8};
 std::unordered_map<std::uint32_t,std::uint32_t> m={{count,bits(1)},{machs,bits(mach)},{reyn,bits(f32(f32(re)*25.0/82.0))},{slot,1},{radius,bits(ler)},{thickness,bits(thick)},{c,bits(chord)},{n,bits(nonlinear)}};
 for(int i=0;i<50;++i){m[xa+4*i]=bits(x[i]);m[pa+4*i]=bits(profile[i]);m[ma+4*i]=bits(0);}
 for(int j=0;j<3;++j)for(int i=0;i<20;++i)m[out[j]+4*i]=bits(kMissing);for(int j=3;j<6;++j)m[out[j]]=bits(0);for(int i=0;i<20;++i)m[out[6]+4*i]=bits(kMissing);
 HybridPressureMachine first(m,&ledger()); auto a=first.run(0x0042F760,{c,n,xa,pa,ma,count,reyn,out[6]}); HybridPressureMachine second(a.memory,&ledger()); auto b=second.run(0x00528448,{title,4,count,machs,reyn,slot,radius,thickness,n,c,xa,pa,ma,out[0],out[1],out[2],out[3],out[4],out[5],out[6]});std::array<double,7> r{};for(int i=0;i<7;++i)r[i]=b.f32(out[i]);return r;
}
}
FinPreprocessingState preprocessFinSectionState(const FinGeometry& g,const FlightCondition& f){
 if(g.sectionFamily!="HEX"||g.npanel!=3&&g.npanel!=4&&g.npanel!=6)throw std::invalid_argument("supported FIN preprocessing requires HEX NPANEL 3, 4, or 6");
 if(f32(g.zupperOverChord[0])!=f32(g.zlowerOverChord[0])||f32(g.zupperOverChord[1])!=f32(g.zlowerOverChord[1])||f32(g.lmaxu[0])!=f32(g.lmaxl[0])||f32(g.lmaxu[1])!=f32(g.lmaxl[1])||f32(g.lflatu[0])||f32(g.lflatu[1])||f32(g.lflatl[0])||f32(g.lflatl[1]))throw std::invalid_argument("unsupported asymmetric or flat HEX section");
 double y0=f32(g.sspanM[0]),y1=f32(g.sspanM[1]),c0=f32(g.chordM[0]),c1=f32(g.chordM[1]),x0=f32(g.xleM[0]),x1=f32(g.xleM[1]),dy=f32(y1-y0);if(dy<=0||c0<=0||c1<=0)throw std::invalid_argument("invalid FIN dimensions");
 FinPreprocessingState s; s.panelAreaM2=f32(f32(c0+c1)*dy/2);s.fullExposedSpanM=f32(2*dy);s.aspectRatio=f32(s.fullExposedSpanM*s.fullExposedSpanM/f32(2*s.panelAreaM2));s.tipRootChordRatio=f32(c1/c0);
 double fraction=f32((c0+f32(2*c1))/f32(3*f32(c0+c1))),ci0=f32(c0/kMPerInternal),ci1=f32(c1/kMPerInternal),chord=f32((2.0/3.0)*(ci0*ci0+ci0*ci1+ci1*ci1)/(ci0+ci1));s.meanAerodynamicSectionChordM=f32(chord*kMPerInternal);
 double half=interp(g.zupperOverChord[0],g.zupperOverChord[1],fraction), xmax=interp(g.lmaxu[0],g.lmaxu[1],fraction);if(!(xmax>0&&xmax<1&&half>0))throw std::invalid_argument("invalid HEX thickness");for(int i=0;i<50;++i){s.profileX[i]=fromBits(kXBits[i]);double q=s.profileX[i];s.profileHalfThickness[i]=q<=xmax?f32(q*(half/xmax)):f32((1-q)*(half/(1-xmax)));s.profileMeanLine[i]=0;}
 s.nonlinearPlanformParameter=f32(f32(profileInterp(s.profileX,s.profileHalfThickness,f32(.06))-profileInterp(s.profileX,s.profileHalfThickness,f32(.0015)))*100);s.sectionAngleDeg=f32(std::atan(half/xmax)*kRadPerDeg);s.leadingEdgeSweepDeg=f32(std::atan(f32(x1-x0)/dy)*kRadPerDeg);s.trailingEdgeSweepDeg=f32(std::atan(f32((x1+c1)-(x0+c0))/dy)*kRadPerDeg);s.halfChordSweepDeg=f32(std::atan(f32((x1+.5*c1)-(x0+.5*c0))/dy)*kRadPerDeg);double ler=effectiveLer(g);s.effectiveLeRadiusM=f32(ler*kMPerInternal);
 if(f32(f.mach)>f32(1.4)){s.sectionKernelOutputs.fill(kMissing);s.sectionLiftSlopePerRad=f32(kMissing*kRadPerDeg);s.nonlinearSectionLimit=kMissing;s.provenance="NATIVE_HIGH_MACH_UNUSED_SECTION_SENTINEL";}else{s.sectionKernelOutputs=kernel(f32(f.mach),f.reynoldsPerM,ler,f32(2*half),s.nonlinearPlanformParameter,chord,s.profileHalfThickness,s.profileX);s.sectionLiftSlopePerRad=f32(s.sectionKernelOutputs[0]*kRadPerDeg);s.nonlinearSectionLimit=s.sectionKernelOutputs[6];s.provenance="SOURCE_DERIVED_NATIVE_AIRFOIL_SECTION_LEDGER";}return s;
}
}
