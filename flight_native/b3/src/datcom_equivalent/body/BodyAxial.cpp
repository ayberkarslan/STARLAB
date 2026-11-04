#include "datcom_equivalent/body/BodyAxial.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include "nlohmann/json.hpp"
#include <unordered_map>
#include <vector>

#include "datcom_equivalent/Validity.hpp"
#include "datcom_equivalent/body/BodyGeometrySupport.hpp"
#include "datcom_equivalent/body/BodyNormal.hpp"
#include "datcom_equivalent/body/BodySupersonic.hpp"
#include "datcom_equivalent/body/BodyWaveProgramData.hpp"
#include "datcom_equivalent/body/HybridPressureMachine.hpp"
#include "datcom_equivalent/body/VanDykeHybrid.hpp"
#include "datcom_equivalent/common/Float32.hpp"

namespace datcom_equivalent::body {
namespace {
using datcom_equivalent::common::f32;

constexpr double kDegPerRad = 57.295780181884766;
// The recovered Python owner stores this DIM-M conversion as REAL*4 before
// every use. Keeping the decimal source literal as binary64 changes the
// critical-Mach skin-friction Reynolds path by one stored REAL*4 ULP.
constexpr double kMetresToInternal = 3.2799999713897705078125;
constexpr double kMetres2ToInternal = 3.2800002;
constexpr double kTiny = 1.0000000031710769e-30;

std::uint32_t bits(double value) {
    float stored = static_cast<float>(f32(value));
    std::uint32_t out;
    std::memcpy(&out, &stored, sizeof(out));
    return out;
}

const HybridPressureMachine::OperationLedger& bodyWaveLedger() {
    static const std::vector<std::pair<std::uint32_t, std::uint32_t>> noTableDwords;
    static const HybridPressureMachine::OperationLedger ledger{
        body_wave_program::kFunctions, body_wave_program::kInitialDwords,
        noTableDwords, body_wave_program::kOperations,
        body_wave_program::kThunks, body_wave_program::kMathIntrinsics};
    return ledger;
}

struct InternalGeometry {
    double nose, diameter, center, total, reference, fineness, noseFineness, wetted;
    double tangentVolume;
    BodyNoseFamily family;
};

void requireSupported(const BodyGeometry& body, const FlightCondition& flight,
                      const ReferenceCondition& reference, double alphaDeg) {
    const double mach = f32(flight.mach);
    const double maximumMach = body.noseFamily == BodyNoseFamily::TangentOgive
        ? f32(1.2) : f32(5.0);
    if (!(f32(0.2) <= mach && mach <= maximumMach)) {
        throw UnsupportedFlightCondition(body.noseFamily == BodyNoseFamily::TangentOgive
            ? "tangent-ogive SUBTRANS BODY axial is validated for stored REAL4 Mach 0.2 through 1.2"
            : "BODY axial is validated for stored REAL4 Mach 0.2 through 5.0");
    }
    if (body.noseLengthM <= 0.0 || body.centerLengthM <= 0.0 || body.diameterM <= 0.0) {
        throw UnsupportedGeometry("sharp BODY nose, cylinder length, and diameter must be positive");
    }
    if (body.noseFamily == BodyNoseFamily::TangentOgive
        && f32(body.noseLengthM) < f32(f32(body.diameterM) / f32(2.0))) {
        throw UnsupportedGeometry("tangent-ogive nose requires LNOSE >= DNOSE/2");
    }
    if (reference.areaM2 <= 0.0 || reference.lengthM <= 0.0) {
        throw UnsupportedGeometry("BODY axial requires positive Sref and Lref");
    }
    if (std::abs(f32(alphaDeg)) > f32(10.0)) {
        throw UnsupportedFlightCondition("BODY axial is validated for alpha between -10 and +10 degrees");
    }
    if (mach > f32(1.2)) validateSupportedSupersonicEnvelope(body, flight, reference, alphaDeg);
}

InternalGeometry internalGeometry(const BodyGeometry& body, const ReferenceCondition& reference) {
    const double nose = f32(f32(body.noseLengthM) * kMetresToInternal);
    const double diameter = f32(f32(body.diameterM) * kMetresToInternal);
    const double center = f32(f32(body.centerLengthM) * kMetresToInternal);
    const double ref = f32(f32(reference.areaM2) * kMetres2ToInternal * kMetres2ToInternal);
    const double total = f32(nose + center);
    const double fineness = f32(total / diameter);
    const double noseFineness = f32(nose / diameter);
    const double half = f32(diameter / 2.0);
    if (body.noseFamily == BodyNoseFamily::TangentOgive) {
        const auto profile = tangentOgiveCylinderProperties(nose, diameter, center);
        return {nose, diameter, center, total, ref, fineness,
                profile.theoreticalNoseFineness, profile.wetted, profile.noseVolume,
                BodyNoseFamily::TangentOgive};
    }
    const double slant = f32(std::sqrt(f32(nose * nose) + f32(half * half)));
    const double wetted = f32(f32(M_PI * half * slant) + f32(M_PI * diameter * center));
    return {nose, diameter, center, total, ref, fineness, noseFineness, wetted, 0.0,
            BodyNoseFamily::SharpCone};
}

double laminarCf(double mach, double reynolds) {
    const double numerator = f32(f32(1.328) / std::sqrt(reynolds));
    return f32(numerator / std::pow(f32(1.0 + f32(0.045) * mach * mach), f32(0.25)));
}

double turbulentCf(double mach, double reynolds) {
    const double machTerm = f32(f32(0.2) * mach * mach);
    const double denominator = f32(f32(machTerm * f32(0.89)) + f32(1.0));
    const double a = f32(machTerm / denominator);
    const double b = f32(f32(f32(machTerm + f32(1.0)) / denominator) - f32(1.0));
    const double radical = std::sqrt(f32(f32(a * f32(4.0)) + f32(b * b)));
    const double betaPlus = f32(f32(f32(a * f32(2.0)) - b) / radical);
    const double betaMinus = f32(b / radical);
    const double shift = f32(std::log10(denominator) * f32(1.26));
    const double angular = f32(f32(f32(std::asin(betaPlus) + std::asin(betaMinus)) * f32(0.242)) /
                               std::sqrt(machTerm));
    double current = f32(f32(0.455) / std::pow(std::log10(reynolds), f32(2.58)));
    for (int iteration = 0; iteration < 10000; ++iteration) {
        const double divisor = std::log10(f32(reynolds * current)) - shift;
        const double updated = f32((angular / divisor) * (angular / divisor));
        const double delta = f32(updated - current);
        if (std::abs(delta) < f32(1.0e-5)) return updated;
        current = f32(current + delta / f32(2.0));
    }
    throw BodyMethodNotImplemented("BODY turbulent skin-friction iteration did not converge");
}

double skinFrictionCa0(const BodyGeometry& body, const FlightCondition& flight,
                       const ReferenceCondition& reference, double mach) {
    const auto g = internalGeometry(body, reference);
    const double unitRe = f32(f32(flight.reynoldsPerM) / kMetresToInternal);
    const double reynolds = f32(g.total * unitRe);
    if (unitRe <= f32(0.1) || reynolds < f32(0.1)) return 0.0;
    const double laminar = laminarCf(mach, reynolds);
    const bool natural = flight.boundaryLayer == "NATURAL";
    double transitionEnd = 0.0;
    double turbulentLength1 = g.total;
    double turbulentLength2 = 0.0;
    if (natural) {
        const double transitionRe = f32(std::exp(f32(mach * f32(0.22))) * f32(1000000.0));
        const double transitionStart = f32(transitionRe / unitRe);
        double constant = f32(f32(f32(0.036) - f32(f32(0.00128) * mach)) - f32(f32(0.00072) * mach * mach));
        if (mach >= f32(3.0)) constant = f32(0.02568);
        if (mach < f32(1.0)) constant = f32(0.034);
        const double endRe = f32(f32(1.0) - f32(std::pow(f32(0.67) / constant, f32(1.25)) /
                                                  std::pow(transitionRe, f32(0.375)))) * transitionRe;
        transitionEnd = f32(endRe / unitRe);
        turbulentLength1 = f32(transitionStart - transitionEnd);
        turbulentLength2 = f32(g.total - transitionStart);
        if (g.total < transitionEnd) return f32(f32(laminar * f32(1.1547000408172607)) * g.wetted / g.reference);
        if (g.total < transitionStart) { turbulentLength2 = 0.0; turbulentLength1 = f32(g.total - transitionEnd); }
    }
    double effectiveRe = reynolds;
    const double roughness = f32(flight.roughnessM * f32(100.0));
    if (roughness != 0.0) {
        const double term = f32(f32(0.2018) * mach + f32(3.60679) + f32(0.12045) * mach * mach);
        const double critical = f32(f32(12.0) * g.total / std::exp((std::log(reynolds) - term) / f32(1.0483)));
        if (roughness >= critical) effectiveRe = f32(std::exp(std::log(f32(12.0) * g.total / roughness) * f32(1.0483) + term));
    }
    const double turbulent = turbulentCf(mach, effectiveRe);
    const double effectiveLaminar = natural ? f32(f32(transitionEnd * laminar) / g.total) : 0.0;
    const double effectiveTurbulent = f32(f32(turbulentLength1 + turbulentLength2) * turbulent / g.total);
    const double combined = f32(f32(effectiveLaminar * f32(1.1547000408172607)) +
                                f32(effectiveTurbulent * f32(1.0219999551773071)));
    return f32(combined * g.wetted / g.reference);
}

HybridPressureMachine::Result runWave(std::uint32_t entry, const std::vector<std::uint32_t>& pointers,
                                      std::unordered_map<std::uint32_t, std::uint32_t> memory) {
    HybridPressureMachine machine(std::move(memory), &bodyWaveLedger());
    return machine.run(entry, pointers);
}

double waveHelper(double fineness, double skin) {
    const std::vector<std::uint32_t> p{0x76000000u, 0x76000400u, 0x76000800u};
    auto result = runWave(0x00411A68u, p, {{p[0], bits(fineness)}, {p[1], bits(skin)}, {p[2], bits(0.0)}});
    return result.f32(p[2]);
}

std::vector<double> contourX(const InternalGeometry& g) {
    if (g.family == BodyNoseFamily::TangentOgive) {
        return tangentOgiveCylinderProperties(g.nose, g.diameter, g.center).contourX;
    }
    std::vector<double> x; x.reserve(21);
    const double noseStep = f32(g.nose / f32(10.0)), centerStep = f32(g.center / f32(10.0));
    for (int i = 0; i <= 10; ++i) x.push_back(f32(noseStep * i));
    for (int i = 1; i <= 10; ++i) x.push_back(f32(g.nose + f32(centerStep * i)));
    return x;
}
std::vector<double> contourR(const InternalGeometry& g) {
    if (g.family == BodyNoseFamily::TangentOgive) {
        return tangentOgiveCylinderProperties(g.nose, g.diameter, g.center).contourR;
    }
    std::vector<double> r; r.reserve(21); const double step = f32(g.diameter / f32(20.0));
    for (int i = 0; i <= 10; ++i) r.push_back(f32(step * i));
    for (int i = 0; i < 10; ++i) r.push_back(f32(g.diameter / f32(2.0)));
    return r;
}

double criticalMach(const InternalGeometry& g, const std::vector<double>& x, const std::vector<double>& r) {
    const std::vector<std::uint32_t> p{0x74000000u,0x74000400u,0x74000800u,0x74000c00u,0x74001000u,0x74001400u};
    std::unordered_map<std::uint32_t,std::uint32_t> m{{p[0],bits(g.noseFineness)},{p[1],bits(x.size())},{p[4],bits(g.nose)},{p[5],bits(0.0)}};
    // MDIV(FRNOSE, NX, R, X, LNOSE, MSUB): radius precedes station.
    for (std::size_t i=0;i<x.size();++i){m[p[2]+4u*i]=bits(r[i]);m[p[3]+4u*i]=bits(x[i]);}
    return runWave(0x004C30A8u,p,std::move(m)).f32(p[5]);
}

double wavePressure(const InternalGeometry& g, double mach, double mcrit, double anchor, double skinCritical,
                    const std::vector<double>& x, const std::vector<double>& r) {
    const std::vector<std::uint32_t> p{0x73000000u,0x73000400u,0x73000800u,0x73000c00u,0x73001000u,0x73001400u,0x73001800u,0x73001c00u,0x73002000u,0x73002400u,0x73002800u};
    const double noseM=f32(g.nose/kMetresToInternal), diaM=f32(g.diameter/kMetresToInternal);
    const double volume = g.family == BodyNoseFamily::TangentOgive
        ? g.tangentVolume
        : f32(f32(M_PI*diaM*diaM*noseM/f32(12.0))*kMetresToInternal*kMetresToInternal*kMetresToInternal);
    const double values[]={f32(x.size()),0.0,0.0,g.reference,mach,g.noseFineness,volume,g.nose,mcrit,anchor,0.0};
    if (const char* path = std::getenv("DATCOM_EQUIVALENT_BODY_AXIAL_PIPELINE"); path != nullptr) {
        nlohmann::json dump;
        dump["mcrit_word"] = bits(mcrit); dump["anchor_word"] = bits(anchor);
        dump["skin_critical_word"] = bits(skinCritical);
        dump["volume_word"] = bits(volume); dump["values_words"] = nlohmann::json::array();
        for (double value : values) dump["values_words"].push_back(bits(value));
        std::ofstream(path) << dump.dump(2);
    }
    std::unordered_map<std::uint32_t,std::uint32_t> m;
    for(std::size_t i=0;i<p.size();++i)m[p[i]]=bits(values[i]);
    for(std::size_t i=0;i<x.size();++i){m[p[1]+4u*i]=bits(x[i]);m[p[2]+4u*i]=bits(r[i]);}
    return runWave(0x00429190u,p,std::move(m)).f32(p[10]);
}

double baseCa(const InternalGeometry& g, double mach) {
    const std::vector<std::uint32_t> p{0x75000000u,0x75000400u,0x75000800u,0x75000c00u,0x75001000u,0x75001400u,0x75001800u};
    std::unordered_map<std::uint32_t,std::uint32_t> m{{p[0],bits(g.diameter)},{p[1],bits(g.diameter)}, {p[2],bits(kTiny)}, {p[3],bits(kTiny)}, {p[4],bits(g.reference)}, {p[5],bits(mach)}, {p[6],bits(0.0)}};
    return runWave(0x004111B8u,p,std::move(m)).f32(p[6]);
}

} // namespace

BodyStaticAxialResult evaluateAxisymmetricBodyStaticAxial(const BodyGeometry& body,
                                                           const FlightCondition& flight,
                                                           const ReferenceCondition& reference,
                                                           double alphaDeg) {
    requireSupported(body, flight, reference, alphaDeg);
    const double mach=f32(flight.mach); const auto g=internalGeometry(body,reference);
    const double skin0=skinFrictionCa0(body,flight,reference,mach);
    double pressure0=0.0, alphaScalar=0.0; std::string pressureMethod;
    if(mach>f32(1.2)) { const auto hybrid=evaluateVanDykeHybridSharpCone(body,flight,reference); pressure0=hybrid.pressureWaveCa0; alphaScalar=hybrid.pressureWaveAlphaScalar; pressureMethod=hybrid.actualMethod; }
    else { const double simple0=waveHelper(g.fineness,skin0); const double simple=mach<f32(1.0)?f32(simple0/std::sqrt(f32(f32(1.0)-f32(mach*mach)))):simple0; const auto x=contourX(g); const auto r=contourR(g); const double mcrit=criticalMach(g,x,r); if(mach<=mcrit){pressure0=simple;pressureMethod="LOW_SUBSONIC_PRANDTL_GLAUERT";}else{const double skinCritical=skinFrictionCa0(body,flight,reference,mcrit);const double anchor=waveHelper(g.fineness,skinCritical);pressure0=wavePressure(g,mach,mcrit,anchor,skinCritical,x,r);pressureMethod="LOW_TRANSONIC_SUB_00429190";} }
    const double base0=baseCa(g,mach); const double ca0=f32(skin0+pressure0); const double radians=f32(f32(alphaDeg)/kDegPerRad); const double sine=std::sin(radians), cosine=std::cos(radians);
    BodyStaticNormalResult normal=evaluateBodyStaticNormalMoment(body,flight,reference,alphaDeg);
    double skin,pressure,base,total,increment;
    if(mach>f32(1.2)){skin=f32(skin0*f32(cosine*cosine));base=f32(base0*cosine);pressure=f32(f32(pressure0+f32(sine*sine)*alphaScalar)*std::sqrt(std::abs(cosine)));total=f32(skin+pressure);increment=f32(total-ca0);} else {const double cross=f32(f32(normal.cnTotal-f32(sine*ca0))/f32(f32(sine*sine)+cosine));total=f32(f32(cosine*f32(ca0+f32(sine*cross)))-f32(sine*cross));increment=f32(total-ca0);skin=f32(skin0*f32(cosine*cosine));pressure=f32(pressure0*f32(cosine*cosine));const double absAlpha=f32(std::abs(f32(alphaDeg)));const double inc=f32(f32(f32(0.012000000104308128)-f32(mach*f32(0.003599999938160181)))*f32(absAlpha/kDegPerRad));base=f32(f32(base0+inc)*cosine);}
    BodyStaticAxialResult out; out.skinFrictionCa=skin;out.pressureWaveCa=pressure;out.baseCa=base;out.finiteAlphaIncrementCa=increment;out.caTotal=total;out.skinFrictionCa0=skin0;out.pressureWaveCa0=pressure0;out.baseCa0=base0;out.activePressureMethod=pressureMethod;out.activeNormalMethod=normal.activeMethod;out.normalMethodFallback=normal.methodFallback;out.validity=body.noseFamily==BodyNoseFamily::TangentOgive?"TANGENT_OGIVE_CONSTANT_DIAMETER_CYLINDER_MACH_0P2_TO_1P2_ALPHA_PLUS_MINUS_10":"SHARP_CONE_CONSTANT_DIAMETER_CYLINDER_MACH_0P2_TO_5_ALPHA_PLUS_MINUS_10";out.warnings={"CA-BASE is report-only and excluded from native BODY CA total."};return out;
}
} // namespace datcom_equivalent::body
