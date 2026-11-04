#include "datcom_equivalent/body/BodyNormal.hpp"

#include <algorithm>
#include <cmath>

#include "datcom_equivalent/Validity.hpp"
#include "datcom_equivalent/body/BodyGeometrySupport.hpp"
#include "datcom_equivalent/body/BodyStaticNormalTables.hpp"
#include "datcom_equivalent/body/CrossflowCdc.hpp"
#include "datcom_equivalent/body/TangentOgiveStaticNormalTables.hpp"
#include "datcom_equivalent/common/Float32.hpp"
#include "datcom_equivalent/common/Interpolation.hpp"

namespace datcom_equivalent::body {

namespace {

using datcom_equivalent::common::f32;
using datcom_equivalent::common::linearInterpolateGeneral;

constexpr double kDegreesPerRadianReal4 = 57.295780181884766;
constexpr double kMetresToInternalLengthReal4 = 3.2799999713897705;
constexpr double kMetres2ToInternalAreaReal4 = 3.2800002098083496;

struct GeometryScalars {
    double area = 0.0;
    double length = 0.0;
    double slenderness = 0.0;
};

// cone_cylinder_crossflow_geometry
GeometryScalars coneCylinderCrossflowGeometry(double noseLength, double diameter,
                                               double centerLength) {
    if (std::min(noseLength, diameter) <= 0.0 || centerLength < 0.0) {
        throw UnsupportedGeometry("invalid cone/cylinder geometry");
    }
    // Single outer f32() wrapping the whole sum -- matches
    // `f32(f32(0.5)*f32(nose_length)*f32(diameter) + f32(center_length)*f32(diameter))`
    // exactly; do not add an intermediate truncation on the first product.
    GeometryScalars g;
    g.area = f32(f32(0.5) * f32(noseLength) * f32(diameter) + f32(centerLength) * f32(diameter));
    g.length = f32(f32(noseLength) + f32(centerLength));
    g.slenderness = f32(g.length * g.length / g.area);
    return g;
}

// cone_cylinder_projected_centroid
double coneCylinderProjectedCentroid(double noseLength, double diameter, double centerLength) {
    const GeometryScalars g = coneCylinderCrossflowGeometry(noseLength, diameter, centerLength);
    const double noseArea = f32(f32(0.5) * f32(noseLength) * f32(diameter));
    const double centerArea = f32(f32(centerLength) * f32(diameter));
    const double numerator =
        noseArea * (f32(2.0) * f32(noseLength) / f32(3.0)) +
        centerArea * (f32(noseLength) + f32(centerLength) / f32(2.0));
    return f32(numerator / g.area);
}

// _tensor: 3-axis nested interpolation, mach (axis2) innermost/fastest.
double tensorInterpolate(const std::vector<double>& axis0, const std::vector<double>& axis1,
                          const std::vector<double>& axis2, const std::vector<double>& values,
                          double q1, double q2, double q3) {
    std::size_t cursor = 0;
    std::vector<double> first;
    first.reserve(axis0.size());
    for (std::size_t i = 0; i < axis0.size(); ++i) {
        std::vector<double> second;
        second.reserve(axis1.size());
        for (std::size_t j = 0; j < axis1.size(); ++j) {
            std::vector<double> row(values.begin() + static_cast<long>(cursor),
                                     values.begin() + static_cast<long>(cursor + axis2.size()));
            cursor += axis2.size();
            second.push_back(linearInterpolateGeneral(axis2, row, q3));
        }
        first.push_back(linearInterpolateGeneral(axis1, second, q2));
    }
    return linearInterpolateGeneral(axis0, first, q1);
}

struct StationTable {
    double station;
    const std::vector<double>& axis0;
    const std::vector<double>& axis1;
    const std::vector<double>& axis2;
    const std::vector<double>& values;
};

// evaluate_sharp_cone_potential_cn_scalar / _cm_scalar share this shape;
// `extraPoint` mirrors CN's extra (10.0, 2.0) tail point that CM does not
// have.
double evaluateSharpConePotentialScalar(double noseFineness, double centerFineness,
                                         double mach, const std::vector<StationTable>& stations,
                                         bool extraPoint) {
    const double machQuery = std::max(f32(mach), f32(0.8));
    std::vector<double> axis;
    std::vector<double> ordinates;
    axis.reserve(stations.size() + 1);
    ordinates.reserve(stations.size() + 1);
    for (const auto& t : stations) {
        const double value = tensorInterpolate(t.axis0, t.axis1, t.axis2, t.values, t.station,
                                                f32(centerFineness), machQuery);
        axis.push_back(f32(t.station));
        ordinates.push_back(f32(value - f32(centerFineness) * f32(0.4)));
    }
    if (extraPoint) {
        axis.push_back(f32(10.0));
        ordinates.push_back(f32(2.0));
    }
    return linearInterpolateGeneral(axis, ordinates, f32(noseFineness));
}

double evaluateSharpConePotentialCnScalar(double noseFineness, double centerFineness,
                                           double mach) {
    using namespace tables;
    const std::vector<StationTable> stations = {
        {0.0, kSharpCnStation0p0_station, kSharpCnStation0p0_centerFineness,
         kSharpCnStation0p0_mach, kSharpCnStation0p0_values},
        {0.5, kSharpCnStation0p5_station, kSharpCnStation0p5_centerFineness,
         kSharpCnStation0p5_mach, kSharpCnStation0p5_values},
        {1.5, kSharpCnStation1p5_station, kSharpCnStation1p5_centerFineness,
         kSharpCnStation1p5_mach, kSharpCnStation1p5_values},
        {2.5, kSharpCnStation2p5_station, kSharpCnStation2p5_centerFineness,
         kSharpCnStation2p5_mach, kSharpCnStation2p5_values},
    };
    return evaluateSharpConePotentialScalar(noseFineness, centerFineness, mach, stations, true);
}

double evaluateSharpConePotentialCmScalar(double noseFineness, double centerFineness,
                                           double mach) {
    using namespace tables;
    const std::vector<StationTable> stations = {
        {0.0, kSharpCmStation0p0_station, kSharpCmStation0p0_centerFineness,
         kSharpCmStation0p0_mach, kSharpCmStation0p0_values},
        {0.5, kSharpCmStation0p5_station, kSharpCmStation0p5_centerFineness,
         kSharpCmStation0p5_mach, kSharpCmStation0p5_values},
        {1.5, kSharpCmStation1p5_station, kSharpCmStation1p5_centerFineness,
         kSharpCmStation1p5_mach, kSharpCmStation1p5_values},
        {2.5, kSharpCmStation2p5_station, kSharpCmStation2p5_centerFineness,
         kSharpCmStation2p5_mach, kSharpCmStation2p5_values},
    };
    return evaluateSharpConePotentialScalar(noseFineness, centerFineness, mach, stations, false);
}

double evaluateTangentOgivePotentialCnScalar(double noseFineness, double centerFineness,
                                              double mach) {
    using namespace tables;
    const std::vector<StationTable> stations = {
        {0.0, kSharpCnStation0p0_station, kSharpCnStation0p0_centerFineness,
         kSharpCnStation0p0_mach, kSharpCnStation0p0_values},
        {0.5, kOgiveStation0p5, kOgiveTfa2, kOgiveMach, kOgiveCnStation0p5Values},
        {1.5, kOgiveStation1p5, kOgiveTfa2, kOgiveMach, kOgiveCnStation1p5Values},
        {2.5, kOgiveStation2p5, kOgiveTfa3, kOgiveMach, kOgiveCnStation2p5Values},
        {3.5, kOgiveStation3p5, kOgiveTfa3, kOgiveMach, kOgiveCnStation3p5Values},
    };
    return evaluateSharpConePotentialScalar(noseFineness, centerFineness, mach, stations, true);
}

double evaluateTangentOgivePotentialCmScalar(double noseFineness, double centerFineness,
                                              double mach) {
    using namespace tables;
    const std::vector<StationTable> stations = {
        {0.0, kSharpCmStation0p0_station, kSharpCmStation0p0_centerFineness,
         kSharpCmStation0p0_mach, kSharpCmStation0p0_values},
        {0.5, kOgiveStation0p5, kOgiveTfa2, kOgiveMach, kOgiveCmStation0p5Values},
        {1.5, kOgiveStation1p5, kOgiveTfa2, kOgiveMach, kOgiveCmStation1p5Values},
        {2.5, kOgiveStation2p5, kOgiveTfa3, kOgiveMach, kOgiveCmStation2p5Values},
        {3.5, kOgiveStation3p5, kOgiveTfa3, kOgiveMach, kOgiveCmStation3p5Values},
    };
    return evaluateSharpConePotentialScalar(noseFineness, centerFineness, mach, stations, false);
}

// evaluate_sharp_cone_potential_cn_coefficient_scalar
double evaluateSharpConePotentialCnCoefficientScalar(double noseLength, double noseBaseDiameter,
                                                       double centerLength,
                                                       double centerDiameter, double mach,
                                                       double referenceArea) {
    if (std::min({noseLength, noseBaseDiameter, centerDiameter, referenceArea}) <= 0.0) {
        throw UnsupportedGeometry("sharp cone scalar inputs must be positive");
    }
    const double raw = evaluateSharpConePotentialCnScalar(
        f32(noseLength) / f32(noseBaseDiameter), f32(centerLength) / f32(centerDiameter), mach);
    return f32(raw * f32(M_PI) * f32(centerDiameter) * f32(centerDiameter) / f32(4.0) /
               f32(referenceArea));
}

// _folded_potential. `alphaDeg` is used both f32-truncated (for magnitude)
// and raw (for the sign check on the line `if alpha_deg < 0.0`) -- exactly
// as the oracle does; do not truncate the sign-check operand.
double foldedPotential(double scalar, double alphaDeg) {
    double magnitude = std::abs(f32(alphaDeg));
    if (magnitude >= f32(90.0)) {
        magnitude = f32(f32(180.0) - magnitude);
    }
    const double radians = magnitude / kDegreesPerRadianReal4;
    double value = (f32(scalar) / 2.0) * std::sin(2.0 * radians) * std::cos(radians / 2.0);
    if (alphaDeg < 0.0) {
        value = -value;
    }
    return f32(value);
}

// evaluate_crossflow_eta ("sub_0049A5E0")
double evaluateCrossflowEta(double slenderness, double mach) {
    using namespace tables;
    const double m = f32(mach);
    if (m > f32(1.4)) {
        return f32(1.0);
    }
    double eta = std::min(f32(1.0),
                           linearInterpolateGeneral(kCrossflowEtaSlenderness, kCrossflowEtaValues,
                                                     f32(slenderness)));
    if (m > f32(0.6) && m < f32(1.4)) {
        const double z = f32(15.0) * (m - f32(1.0)) / (m * m * m * m);
        eta = f32(eta + ((f32(1.0) - eta) / f32(2.0)) * (std::tanh(z) + f32(1.0)));
    }
    return eta;
}

}  // namespace

BodyStaticNormalResult evaluateAxisymmetricBodyStaticNormal(const BodyGeometry& body,
                                                              const FlightCondition& flight,
                                                              const ReferenceCondition& reference,
                                                              double alphaDeg) {
    const double noseLength = body.noseLengthM;
    const double diameter = body.diameterM;
    const double centerLength = body.centerLengthM;
    const double noseOrigin = body.noseOriginM;

    const double mach = f32(flight.mach);
    if (mach > f32(1.2)) {
        throw BodyMethodNotImplemented("M>1.2 BODY method routes are not recovered");
    }

    const double sref = reference.areaM2;
    const double lref = reference.lengthM;
    const double xcg = reference.xcgM;

    GeometryScalars geom;
    double centroid = 0.0;
    double potentialCnScalar = 0.0;
    double rawCmPosition = 0.0;
    if (body.noseFamily == BodyNoseFamily::SharpCone) {
        geom = coneCylinderCrossflowGeometry(noseLength, diameter, centerLength);
        centroid = coneCylinderProjectedCentroid(noseLength, diameter, centerLength);
        potentialCnScalar = evaluateSharpConePotentialCnCoefficientScalar(
            noseLength, diameter, centerLength, diameter, mach, sref);
        rawCmPosition = evaluateSharpConePotentialCmScalar(
            f32(noseLength) / f32(diameter), f32(centerLength) / f32(diameter), mach);
    } else {
        const double noseInternal = f32(f32(noseLength) * kMetresToInternalLengthReal4);
        const double diameterInternal = f32(f32(diameter) * kMetresToInternalLengthReal4);
        const double centerInternal = f32(f32(centerLength) * kMetresToInternalLengthReal4);
        const auto profile = tangentOgiveCylinderProperties(
            noseInternal, diameterInternal, centerInternal);
        geom.area = profile.planform;
        geom.length = f32(noseInternal + centerInternal);
        geom.slenderness = f32(geom.length * geom.length / geom.area);
        centroid = f32(profile.planformCentroid / kMetresToInternalLengthReal4);
        const double centerFineness = f32(centerInternal / diameterInternal);
        const double rawCn = evaluateTangentOgivePotentialCnScalar(
            profile.theoreticalNoseFineness, centerFineness, mach);
        const double referenceInternal = f32(f32(sref) * kMetres2ToInternalAreaReal4 *
                                             kMetres2ToInternalAreaReal4);
        potentialCnScalar = f32(rawCn * f32(M_PI) * diameterInternal * diameterInternal /
                                f32(4.0) / referenceInternal);
        rawCmPosition = evaluateTangentOgivePotentialCmScalar(
            profile.theoreticalNoseFineness, centerFineness, mach);
    }
    // 0x0041C880..0x0041C8F2 for the single nonzero sharp-cone contribution.
    const double potentialCmScalar =
        f32(potentialCnScalar * ((f32(xcg) - f32(noseOrigin)) - rawCmPosition * f32(diameter)) /
            f32(lref));

    // The 0x630B1C owner selects the Reynolds-dependent CDC route for the
    // NATURAL boundary-layer BODY normal calculation. BODY-FIN BDWK
    // retains its separately proved non-one/selector-zero contract.
    std::string boundaryLayer = flight.boundaryLayer;
    std::transform(boundaryLayer.begin(), boundaryLayer.end(), boundaryLayer.begin(),
                    [](unsigned char c) { return std::toupper(c); });
    const double viscousSelector = (boundaryLayer == "NATURAL") ? 1.0 : 0.0;
    const auto viscousCdc = evaluateBodyCrossflowCdc(viscousSelector, body, flight.reynoldsPerM,
                                                      flight.mach, alphaDeg);
    const auto bdwkCdc =
        evaluateBodyCrossflowCdc(0.0, body, flight.reynoldsPerM, flight.mach, alphaDeg);

    const double eta = evaluateCrossflowEta(geom.slenderness, mach);
    const double cdc = viscousCdc.cdc;
    const double viscousMomentX2 = f32(noseOrigin + centroid);

    const double potentialCn = foldedPotential(potentialCnScalar, alphaDeg);
    const double sine = std::sin(f32(alphaDeg) / kDegreesPerRadianReal4);
    const double normalReferenceArea = body.noseFamily == BodyNoseFamily::SharpCone
        ? f32(sref) : f32(f32(sref) * kMetres2ToInternalAreaReal4 * kMetres2ToInternalAreaReal4);
    const double viscousCn =
        f32(f32(cdc) * f32(eta) * f32(geom.area) / normalReferenceArea * sine * std::abs(sine));
    const double potentialCm = foldedPotential(potentialCmScalar, alphaDeg);
    const double viscousCm = f32(f32(cdc) * f32(eta) * f32(geom.area) *
                                  (f32(xcg) - f32(viscousMomentX2)) / f32(lref) * sine *
                                  std::abs(sine) / normalReferenceArea);

    const double cnTotal = f32(potentialCn + viscousCn);
    const double cmTotal = f32(potentialCm + viscousCm);

    BodyStaticNormalResult result;
    result.potentialCn = potentialCn;
    result.viscousCn = viscousCn;
    result.cnTotal = cnTotal;
    result.potentialCm = potentialCm;
    result.viscousCm = viscousCm;
    result.cmTotal = cmTotal;
    result.bdwk = bdwkCdc.cdc;
    result.bodyCp = (cnTotal == 0.0) ? std::nullopt : std::optional<double>(f32(cmTotal / cnTotal));
    result.activeMethod = body.noseFamily == BodyNoseFamily::SharpCone
        ? "SHARP_CONE_CYLINDER_M_LE_1P2"
        : "TANGENT_OGIVE_CYLINDER_STBODY_M_LE_1P2";
    result.methodFallback = false;
    result.validity = body.noseFamily == BodyNoseFamily::SharpCone
        ? "RECOVERED_RAW_GEOMETRY_SUBROUTE"
        : "TANGENT_OGIVE_SUBTRANS_RECOVERED_OWNER_ROUTE";
    result.warnings = {"BODY axial force is excluded"};
    return result;
}

}  // namespace datcom_equivalent::body
