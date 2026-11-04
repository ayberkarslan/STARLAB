#include "datcom_equivalent/body_fin/BodyFinFinite.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

#include "datcom_equivalent/common/Float32.hpp"
#include "datcom_equivalent/body/BodyGeometrySupport.hpp"
#include "datcom_equivalent/fin/IsolatedFinStatic.hpp"

namespace datcom_equivalent::body_fin {
namespace {
using common::f32;
constexpr double kRad = 57.295780181884766;
constexpr double kPi = 3.1415927410125732;
constexpr double kUnitM = .304878;

const std::array<double, 10> kKwMachAxis = {0., .6000000238418579, .8999999761581421,
    1.2000000476837158, 1.600000023841858, 2., 2.299999952316284,
    2.9600000381469727, 3.950000047683716, 5.};
const std::array<double, 10> kKwMachExponent = {11.168999671936035, 11.168999671936035,
    11.28499984741211, 9.847999572753906, 8.083000183105469, 8.824999809265137,
    8.28499984741211, 7.681000232696533, 7.888999938964844, 7.888999938964844};
const std::array<double, 5> kKwRadiusAxis = {0., .25, .33329999446868896, .5, 1.};
const std::array<double, 5> kKwRadiusIncrement = {0., .09399999678134918,
    .10300000011920929, .0989999994635582, 0.};
const std::array<double, 11> kEtaAxis = {0., .1, .2, .3, .4, .5, .6, .7, .8, .9, 1.};
const std::array<double, 11> kEtaTableA = {.637, .687, .681, .649, .597, .529, .447, .352, .246, .128, 0.};
const std::array<double, 11> kEtaTableB = {.382, .447, .490, .508, .502, .471, .417, .342, .244, .127, 0.};
const std::array<double, 11> kBodyAxis = {0.,1.,2.,3.,4.,5.,6.,7.,8.,9.,10.};
const std::array<double, 11> kTableC = {1.,1.17,1.29,1.44,1.58,1.70,1.83,1.97,2.05,2.15,2.20};
const std::array<double, 11> kTableLowA = {.405,.43,.45,.48,.495,.505,.525,.55,.565,.59,.595};
const std::array<double, 11> kTableLowB = {.31,.31,.33,.40,.46,.60,.80,.90,1.,1.1,1.15};
const std::array<double, 11> kTableHighA = {.60,.605,.615,.65,.68,.695,.70,.705,.725,.73,.73};
const std::array<double, 11> kTableHighB = {.58,.60,.62,.79,.88,1.05,1.25,1.35,1.50,1.55,1.60};
const std::array<double, 20> kPanelAlphaAxis = {0.,2.,4.,6.,8.,10.,12.,14.,16.,18.,20.,24.,28.,32.,36.,40.,44.,48.,52.,56.};

template <std::size_t N>
double linearNative(double q, const std::array<double,N>& x, const std::array<double,N>& y) {
    const double value=f32(q); std::size_t i=0;
    while(i+1<N && value>f32(x[i+1])) ++i;
    i=std::min(i,N-2);
    return f32(f32(y[i])+(value-f32(x[i]))*(f32(y[i+1])-f32(y[i]))/(f32(x[i+1])-f32(x[i])));
}

template <std::size_t N>
double naturalCubic(const std::array<double,N>& axis, const std::array<double,N>& values, double query) {
    std::array<double,N> y2{}; std::array<double,N-1> u{};
    for(std::size_t i=1;i+1<N;++i){
        const double sig=(f32(axis[i])-f32(axis[i-1]))/(f32(axis[i+1])-f32(axis[i-1]));
        const double p=sig*y2[i-1]+2.; y2[i]=(sig-1.)/p;
        u[i]=(6.*((f32(values[i+1])-f32(values[i]))/(f32(axis[i+1])-f32(axis[i]))-
                  (f32(values[i])-f32(values[i-1]))/(f32(axis[i])-f32(axis[i-1]))) /
                  (f32(axis[i+1])-f32(axis[i-1]))-sig*u[i-1])/p;
    }
    for(std::size_t k=N-1;k-- > 0;) y2[k]=y2[k]*y2[k+1]+u[k];
    const double q=f32(query); std::size_t hi=1; while(hi<N && q>f32(axis[hi])) ++hi;
    if(hi==N) hi=N-1; const std::size_t lo=hi-1;
    const double h=f32(axis[hi])-f32(axis[lo]), a=(f32(axis[hi])-q)/h, b=(q-f32(axis[lo]))/h;
    return f32(a*f32(values[lo])+b*f32(values[hi])+((a*a*a-a)*y2[lo]+(b*b*b-b)*y2[hi])*h*h/6.);
}

double panelFold(double phi) { double out=std::abs(f32(phi)); if(out>=180.)out=f32(360.-out); return f32(90.-out); }

double geometryIntegral(double c1,double c2,double cosine,double sine,double outer,double inner,double ratio) {
    double x=f32(c1),y=f32(c2),c=f32(cosine),s=f32(sine),ro=f32(outer),ri=f32(inner),tr=f32(ratio);
    std::array<double,4> terms{};
    for(int orientation=0;orientation<4;++orientation){
        double along=0.,normal=0.;
        if(orientation==0){along=f32(x*c+y*s);normal=f32(y*c-x*s);}
        else if(orientation==1){normal=f32(x*s+y*c);along=f32(y*s-x*c);}
        else if(orientation==2){double den=f32(y*y+x*x);if(den==0.)den=f32(1.e-4);y=f32(y*ri*ri/den);x=f32(x*ri*ri/den);along=f32(x*c+y*s);normal=f32(y*c-x*s);}
        else {normal=f32(x*s+y*c);along=f32(y*s-x*c);}
        if(normal==0.&&along==ro)return f32(-1.e10);
        const double leading=f32(((ro-ri*tr)-along*(1.-tr))/(2.*(ro-ri)));
        const double logarithm=std::log((normal*normal+(ro-along)*(ro-along))/(normal*normal+(ri-along)*(ri-along)));
        const double logTerm=f32(logarithm*leading);
        const double divisor=normal==0.?f32(1.e-4):normal;
        (void)divisor;
        const double uo=f32((along-ro)/normal),ui=f32((along-ri)/normal);
        double angular=f32((ro-ri)+std::atan(std::abs(uo))*normal*(uo<0.?-1.:1.));
        angular=f32(angular-std::atan(std::abs(ui))*normal*(ui<0.?-1.:1.));
        angular=f32((1.-tr)*angular/(ro-ri)); terms[orientation]=f32(logTerm-angular);
    }
    return f32(2.*(terms[0]-terms[1]-terms[2]+terms[3])/(tr+1.));
}

struct BodyHelper { double angle=0.; std::string branch; };
BodyHelper bodyFlow(double mach,double bodyOperand,double epsilon,double chordRatio,double geometryAngle,
                    double rootChord,double radius,double aspect,double radiusOverEta,double taper,
                    double foldedPhi,double absoluteAlpha) {
    const double a1=f32(mach),a2=f32(bodyOperand),a3=f32(epsilon),a4=f32(chordRatio),a5=f32(geometryAngle),a6=f32(rootChord),a7=f32(radius),a8=f32(aspect),a9=f32(radiusOverEta),a10=f32(taper),phi=f32(foldedPhi),alpha=f32(absoluteAlpha);
    const double sine=f32(std::sin(phi/kRad)),cosine=f32(std::cos(phi/kRad)),sinAlpha=f32(std::sin(alpha/kRad));
    if(alpha<=f32(4.))return {0.,"ALPHA_LE_4"};
    double station=0.; std::string stationBranch;
    if(a3<=f32(.2)&&a5<=f32(30.)){station=alpha<a5?f32((1.-std::sqrt((alpha-4.)/(a5-4.)))*32.*a7):0.;stationBranch="SPECIAL_GEOMETRY";}
    else {station=f32((2.+10./(alpha-4.))*a7);stationBranch="GENERAL_GEOMETRY";}
    const double query=f32((a6-station)*sinAlpha/a7);
    if(query<=0.)return {0.,stationBranch+"+QUERY_NONPOSITIVE"};
    const double valueC=f32(linearNative(query,kBodyAxis,kTableC)*a4);
    const double lowA=f32(linearNative(query,kBodyAxis,kTableLowA)*a4),lowB=f32(linearNative(query,kBodyAxis,kTableLowB)*a7/a9);
    const double highA=f32(linearNative(query,kBodyAxis,kTableHighA)*a4),highB=f32(linearNative(query,kBodyAxis,kTableHighB)*a7/a9);
    double valueA=0.,valueB=0.;std::string machBranch;
    if(a1<=1.){valueA=lowA;valueB=lowB;machBranch="MACH_LE_1";}
    else if(a1<2.){valueA=f32(lowA+(highA-lowA)*(a1-1.));valueB=f32(lowB+(highB-lowB)*(a1-1.));machBranch="MACH_1_TO_2";}
    else {valueA=highA;valueB=highB;machBranch="MACH_GE_2";}
    std::string queryBranch;
    if(query<4.){const double spanWork=f32((a6-station)*(a4+a7)),squares=valueA*valueA+valueC*valueC;
        valueB=f32(a2*sinAlpha*spanWork/(kPi*8.*a9*valueA*(1.-a4*a4/squares)));queryBranch="QUERY_LT_4";}
    else queryBranch="QUERY_GE_4";
    const double integral=geometryIntegral(valueA,valueC,cosine,sine,a9,a7,a10);
    const double bounded=std::max(-1.,std::min(1.,f32(valueB*integral)));
    return {f32(std::asin(sinAlpha*bounded)),stationBranch+"+"+machBranch+"+"+queryBranch};
}

EquivalentPanelAngleResult equivalentAngle(double eta,double alpha,double ar,double phi,double mach,double length,
    double chordRatio,double root,double radius,double taper,double kw,double bodyOperand,double epsilon,double sweep,double area) {
    eta=f32(eta);alpha=f32(alpha);ar=f32(ar);phi=f32(phi);mach=f32(mach);length=f32(length);chordRatio=f32(chordRatio);root=f32(root);radius=f32(radius);taper=f32(taper);kw=f32(kw);bodyOperand=f32(bodyOperand);epsilon=f32(epsilon);sweep=f32(sweep);area=f32(area);
    const double folded=panelFold(phi),signedFolded=f32(alpha<0.?-folded:folded),lengthSquarePlus=f32(length*length+f32(.25));
    const double geometryCosine=f32(std::sqrt(1.-length*length/(lengthSquarePlus*lengthSquarePlus)));
    const double geometryAngle=f32(std::acos(geometryCosine)*kRad),radiusOverEta=f32(radius/eta);
    auto helper=bodyFlow(mach,bodyOperand,epsilon,chordRatio,geometryAngle,root,radius,ar,radiusOverEta,taper,signedFolded,std::abs(alpha));
    double helperAngle=helper.angle;if(alpha<0.)helperAngle=-helperAngle;if(phi<0.||phi>=180.)helperAngle=-helperAngle;helperAngle=f32(helperAngle);
    const double alphaRad=f32(alpha/kRad),phiRad=f32(phi/kRad),table=naturalCubic(kEtaAxis,kEtaTableB,eta),sweepRad=f32(sweep/kRad);
    const double scale=f32(radius*radius*(eta-1.)*(eta-1.)*std::tan(sweepRad)*std::tan(sweepRad)/(2.*eta*eta*area));
    const double sa=std::sin(alphaRad),ca=std::cos(alphaRad),sp=std::sin(phiRad),cp=std::cos(phiRad);
    const double tangent=f32(std::tan(helperAngle)+(kw*sa*sp-table*scale*sa*sa*sp*cp)/std::abs(ca));
    return {f32(std::atan(tangent)*kRad),helperAngle,table,scale,helper.branch};
}
} // namespace

double evaluateKwAlphaScale(double mach,double alphaDeg,double radiusM,double spanM,double kwBase) {
    if(radiusM<0.||spanM<=0.)throw std::invalid_argument("body radius must be nonnegative and exposed semispan positive");
    if(kwBase<=1.)return 1.;
    const double ratio=f32(radiusM/(radiusM+spanM));
    const double exponent=linearNative(f32(mach),kKwMachAxis,kKwMachExponent),increment=linearNative(ratio,kKwRadiusAxis,kKwRadiusIncrement);
    const double amplitude=f32(increment+kwBase-1.),inverse=f32(1./std::pow(exponent,1.5));
    const double decay=f32(std::log(1.+increment/(kwBase-1.))*inverse);
    return f32(f32(1.+amplitude*std::exp(-(std::pow(std::abs(alphaDeg),1.5))*decay))/kwBase);
}

BodyFinFiniteResult evaluateBodyFinFiniteAlpha(const BodyGeometry& body,const FinGeometry& fin,const FlightCondition& flight,
    const ReferenceCondition& reference,const fin::FinPreprocessingState& state,const BodyFinBaseResult& base,double alphaDeg,double bdwk) {
    if(fin.sspanM[1]<=fin.sspanM[0]||fin.chordM[0]<=0.||fin.chordM[1]<0.)throw std::invalid_argument("unsupported BODY-FIN finite-alpha request");
    const double mach=f32(flight.mach),alpha=f32(alphaDeg),xr=f32(fin.xleM[0]),xt=f32(fin.xleM[1]),spanM=f32(f32(fin.sspanM[1])-f32(fin.sspanM[0])),crM=f32(fin.chordM[0]),ctM=f32(fin.chordM[1]);
    const double radiusM=f32(body::bodyRadiusAtStation(body,f32(xr+f32(crM/2.)))),span=f32(spanM/f32(kUnitM)),radius=f32(radiusM/f32(kUnitM)),centerRadius=f32(f32(.5*body.diameterM)/f32(kUnitM)),rootStation=f32(xr/f32(kUnitM)),cr=f32(crM/f32(kUnitM)),ct=f32(ctM/f32(kUnitM));
    const double eta=f32(radius/f32(radius+span)),ar=f32(4.*span/f32(cr+ct)),taper=f32(ct/cr),sweep=f32(std::atan(f32(f32(xt-xr)/spanM))*kRad),area=f32(f32(spanM*f32(crM+ctM)/2.)/f32(f32(kUnitM)*f32(kUnitM)));
    // PORT-004: SYNTHS passes LODN=GEOB(6), the nose fineness ratio, to
    // PANLCN/ALPEQ for every supported nose family (synths.f:138,472-477).
    // PORT-003 fixed only tangent ogives and intentionally left the frozen
    // sharp-cone oracle path unchanged; the same binding defect was therefore
    // still present for cones above the ALPEQ alpha=4 deg body-flow threshold.
    const double noseFineness=f32(f32(body.noseLengthM)/f32(body.diameterM));
    BodyFinFiniteResult out;out.kwAlphaScale=evaluateKwAlphaScale(mach,alpha,radiusM,spanM,base.kwBase);out.kwAlpha=f32(base.kwBase*out.kwAlphaScale);out.eta=eta;out.bdwk=f32(bdwk);out.activeUpwashMethod="sub_004CC848+sub_004094F0:first_finset_static_arg30_0";
    std::array<double,20> curve{};for(std::size_t i=0;i<curve.size();++i)curve[i]=f32(2.*fin::evaluateIsolatedFinPanelNormalForce(fin,state,flight,reference,kPanelAlphaAxis[i]).cn);
    double total=0.;int index=0;for(double rawPhi:fin.panelAzimuthsDeg){const double phi=f32(rawPhi);auto upwash=equivalentAngle(eta,alpha,ar,phi,mach,noseFineness,centerRadius,rootStation,radius,taper,out.kwAlpha,bdwk,f32(1.e-30),sweep,area);const double twice=naturalCubic(kPanelAlphaAxis,curve,std::abs(upwash.equivalentAlphaDeg));const double panel=f32((upwash.equivalentAlphaDeg<0.?-1.:1.)*twice/2.);out.panels.push_back({++index,phi,upwash.equivalentAlphaDeg,panel,upwash});total=f32(total+panel*std::sin(phi/kRad));}
    out.finInBodyCn=total;return out;
}
} // namespace datcom_equivalent::body_fin
