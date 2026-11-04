#include "datcom_equivalent/body/CrossflowCdc.hpp"

#include <cmath>
#include <vector>

#include "datcom_equivalent/Validity.hpp"
#include "datcom_equivalent/body/BodyGeometrySupport.hpp"
#include "datcom_equivalent/common/Float32.hpp"
#include "datcom_equivalent/common/Interpolation.hpp"

namespace datcom_equivalent::body {

namespace {

using datcom_equivalent::common::f32;
using datcom_equivalent::common::linearInterpolate;

// Exact REAL*4 literals exported from 0x00591484..0x005916B7, copied
// verbatim (same decimal digit strings) from crossflow_cdc.py so both
// languages parse the identical IEEE-754 binary64 value at compile/parse
// time.
constexpr double kDegreesPerRadianReal4 = 57.295780181884766;
constexpr double kMetresToInternalLengthReal4 = 3.2799999713897705;

const std::vector<double> kSelector1MachAxis = {
    0.0, 0.25, 0.30000001192092896, 0.3499999940395355, 0.4000000059604645};

const std::vector<double> kSelector1LogReAxis = {
    6.0, 6.301000118255615, 6.477099895477295, 6.602099895477295,
    6.698999881744385, 6.778200149536133, 6.845099925994873,
    6.90310001373291, 6.95419979095459, 7.0};

const std::vector<std::vector<double>> kSelector1Tensor = {
    {0.20000000298023224, 0.44999998807907104, 0.5400000214576721, 0.5899999737739563,
     0.6000000238418579, 0.5899999737739563, 0.5799999833106995, 0.550000011920929,
     0.5299999713897705, 0.5199999809265137},
    {0.20000000298023224, 0.44999998807907104, 0.5400000214576721, 0.5899999737739563,
     0.6000000238418579, 0.5899999737739563, 0.5799999833106995, 0.550000011920929,
     0.5299999713897705, 0.5199999809265137},
    {0.30000001192092896, 0.47999998927116394, 0.5600000023841858, 0.6000000238418579,
     0.6200000047683716, 0.6100000143051147, 0.6050000190734863, 0.6000000238418579,
     0.5899999737739563, 0.5799999833106995},
    {0.30000001192092896, 0.44999998807907104, 0.5299999713897705, 0.6000000238418579,
     0.6399999856948853, 0.6899999976158142, 0.699999988079071, 0.6399999856948853,
     0.6200000047683716, 0.6000000238418579},
    {0.5, 0.5299999713897705, 0.5699999928474426, 0.6000000238418579, 0.6399999856948853,
     0.6899999976158142, 0.7599999904632568, 0.8799999952316284, 0.6700000166893005,
     0.6299999952316284},
};

const std::vector<double> kSelector1LinearLogReAxis = {
    4.0, 4.544099807739258, 4.95419979095459, 5.17609977722168, 5.556300163269043,
    5.602099895477295, 5.698999881744385, 5.778200149536133, 5.845099925994873,
    5.95419979095459, 6.0, 7.0};

const std::vector<double> kSelector1LinearValues = {
    1.2000000476837158, 1.2000000476837158, 1.2200000286102295, 1.2200000286102295,
    0.7200000286102295, 0.6299999952316284, 0.25999999046325684, 0.25,
    0.25999999046325684, 0.2800000011920929, 0.28999999165534973, 0.6399999856948853};

const std::vector<double> kCurveBAxis = {
    0.0, 0.10000000149011612, 0.20000000298023224, 0.30000001192092896,
    0.4000000059604645, 0.5};

const std::vector<double> kCurveBValues = {
    0.2800000011920929, 0.3799999952316284, 0.47999998927116394, 0.7400000095367432,
    1.0, 1.3600000143051147};

const std::vector<double> kCurveAAxis = {
    0.0, 0.4000000059604645, 0.5, 0.6000000238418579, 0.699999988079071,
    0.800000011920929, 0.8999999761581421, 1.0, 1.100000023841858,
    1.2000000476837158, 1.2999999523162842, 1.399999976158142, 1.5,
    1.600000023841858, 1.7999999523162842, 2.0, 2.200000047683716,
    2.799999952316284, 4.0, 4.800000190734863};

const std::vector<double> kCurveAValues = {
    1.2000000476837158, 1.2899999618530273, 1.3600000143051147, 1.5, 1.5, 1.5,
    1.409999966621399, 1.350000023841858, 1.3200000524520874, 1.2799999713897705,
    1.25, 1.2300000190734863, 1.2100000381469727, 1.2000000476837158, 1.2000000476837158,
    1.2000000476837158, 1.2000000476837158, 1.2000000476837158, 1.2000000476837158,
    1.2000000476837158};

double selector1Tensor(double crossflowMach, double crossflowReynolds) {
    const double logRe = f32(std::log10(crossflowReynolds));
    std::vector<double> byMach;
    byMach.reserve(kSelector1Tensor.size());
    for (const auto& row : kSelector1Tensor) {
        byMach.push_back(linearInterpolate(kSelector1LogReAxis, row, logRe));
    }
    return linearInterpolate(kSelector1MachAxis, byMach, crossflowMach);
}

}  // namespace

CrossflowCdcResult evaluateCrossflowCdcOperands(double selector,
                                                 double alphaDeg,
                                                 double reynoldsPerInternalLength,
                                                 double geometryScale,
                                                 double mach) {
    const double alpha = f32(alphaDeg);
    const double machF = f32(mach);
    const double selectorF = f32(selector);
    const double angle = std::abs(alpha) / kDegreesPerRadianReal4;
    const double crossflowMach = f32(std::sin(angle) * machF);
    const double product = f32(f32(reynoldsPerInternalLength) * f32(geometryScale));
    const double crossflowReynolds = f32(std::sin(angle) * product);

    double cdc = 0.0;
    std::string branch;

    if (selectorF == f32(1.0)) {
        if (crossflowMach <= f32(0.4)) {
            if (crossflowReynolds <= f32(10000.0)) {
                cdc = f32(1.2);
                branch = "SELECTOR1_MCF_LE_0P4_RECF_LE_1E4_CONSTANT";
            } else if (crossflowReynolds <= f32(1000000.0)) {
                cdc = linearInterpolate(kSelector1LinearLogReAxis, kSelector1LinearValues,
                                         f32(std::log10(crossflowReynolds)));
                branch = "SELECTOR1_MCF_LE_0P4_LOGRE_LINEAR";
            } else if (crossflowReynolds <= f32(10000000.0)) {
                cdc = selector1Tensor(crossflowMach, crossflowReynolds);
                branch = "SELECTOR1_MCF_LE_0P4_LOGRE_MCF_TENSOR";
            } else {
                cdc = linearInterpolate(kCurveAAxis, kCurveAValues, crossflowMach);
                branch = "SELECTOR1_MCF_LE_0P4_HIGH_RE_CURVE_A";
            }
        } else if (crossflowMach < f32(0.5) && crossflowReynolds > f32(100000.0)) {
            cdc = linearInterpolate(kCurveBAxis, kCurveBValues, crossflowMach);
            branch = "SELECTOR1_0P4_MCF_LT_0P5_HIGH_RE_CURVE_B";
        } else {
            cdc = linearInterpolate(kCurveAAxis, kCurveAValues, crossflowMach);
            branch = "SELECTOR1_HIGH_MCF_OR_LOW_RE_CURVE_A";
        }
    } else {
        const double curveA = linearInterpolate(kCurveAAxis, kCurveAValues, crossflowMach);
        const double curveB = linearInterpolate(kCurveBAxis, kCurveBValues, crossflowMach);
        if (crossflowMach >= f32(0.5)) {
            cdc = curveA;
            branch = "SELECTOR_NON1_SELECTOR0_CURVE_A";
        } else {
            cdc = f32((curveA - curveB) * f32(0.5) + curveB);
            branch = "SELECTOR_NON1_SELECTOR0_BLEND";
        }
    }

    CrossflowCdcResult result;
    result.cdc = f32(cdc);
    result.selector = selectorF;
    result.crossflowMach = crossflowMach;
    result.reynoldsOperand = f32(reynoldsPerInternalLength);
    result.reynoldsCrossflow = crossflowReynolds;
    result.geometryScale = f32(geometryScale);
    result.activeBranch = branch;
    result.validity = "SHARP_CONE_CONSTANT_DIAMETER_CYLINDER_STATIC_ALPHA";
    return result;
}

CrossflowCdcResult evaluateBodyCrossflowCdc(double selector,
                                             const BodyGeometry& body,
                                             double reynoldsPerM,
                                             double mach,
                                             double alphaDeg) {
    const double nose = f32(body.noseLengthM * kMetresToInternalLengthReal4);
    const double diameter = f32(body.diameterM * kMetresToInternalLengthReal4);
    const double center = f32(body.centerLengthM * kMetresToInternalLengthReal4);
    if (nose <= 0.0 || diameter <= 0.0 || center < 0.0) {
        throw UnsupportedGeometry("invalid cone/cylinder geometry");
    }
    // BodyGeometry has a single diameterM shared by nose base and center.
    // Tangent-ogive planform follows OGIVE's 101-point DSPLAN integration.
    const double planform = body.noseFamily == BodyNoseFamily::SharpCone
        ? f32(f32(f32(0.5) * nose) * diameter + center * diameter)
        : tangentOgiveCylinderProperties(nose, diameter, center).planform;
    const double length = f32(nose + center);
    const double geometryScale = f32(planform / length);
    const double reynoldsPerMF = f32(reynoldsPerM);
    return evaluateCrossflowCdcOperands(
        selector, alphaDeg, f32(reynoldsPerMF / kMetresToInternalLengthReal4),
        geometryScale, mach);
}

}  // namespace datcom_equivalent::body
