#pragma once

// Raw request/result domain types for the native DATCOM Equivalent provider.
//
// These mirror, field-for-field, the dataclasses in the frozen Python oracle
// package (third_party/datcom_equivalent_oracle/datcom_equivalent/):
//   - BodyGeometry / FlightCondition / ReferenceCondition
//     (datcom_equivalent/provider/single_finset.py)
//   - IsolatedFinGeometrySI (datcom_equivalent/fin/isolated.py)
//   - DatcomEquivalentRequest (datcom_equivalent/request.py)
//   - SingleFinsetStaticResult (datcom_equivalent/provider/single_finset.py)
//
// No aerodynamic coefficient may be supplied as an input (oracle request
// contract 2.0.0). This header intentionally carries no preprocessing state
// (section lift slope, nonlinear limit, etc.) in the public request types.

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace datcom_equivalent {

enum class BodyNoseFamily {
    SharpCone,
    TangentOgive,
};

// Sharp-nose-plus-constant-diameter-cylinder BODY geometry, SI units.
// TangentOgive is available only through the separately gated SUBTRANS
// operation; its Mach-greater-than-1.20 owner family remains fail-closed.
struct BodyGeometry {
    double noseLengthM = 0.0;
    double diameterM = 0.0;
    double centerLengthM = 0.0;
    double noseOriginM = 0.0;
    BodyNoseFamily noseFamily = BodyNoseFamily::SharpCone;
};

// Two-station FIN geometry: lengths in metres; section ordinates and locations
// are dimensionless chord fractions. Mirrors IsolatedFinGeometrySI semantics.
// Only section_family == "HEX" is supported end-to-end.
struct FinGeometry {
    std::array<double, 2> sspanM{0.0, 0.0};
    std::array<double, 2> chordM{0.0, 0.0};
    std::array<double, 2> xleM{0.0, 0.0};
    std::string sectionFamily;                 // "HEX" (only supported value)
    std::array<double, 2> zupperOverChord{0.0, 0.0};
    std::array<double, 2> zlowerOverChord{0.0, 0.0};
    std::vector<double> panelAzimuthsDeg{45.0, 135.0, 225.0, 315.0};
    int npanel = 4;                             // supported: 3, 4, 6
    // LMAXU/LMAXL: maximum-thickness location / chord; LFLATU/LFLATL: flat length / chord.
    std::array<double, 2> lmaxu{0.5, 0.5};
    std::array<double, 2> lmaxl{0.5, 0.5};
    std::array<double, 2> lflatu{0.0, 0.0};
    std::array<double, 2> lflatl{0.0, 0.0};
    std::vector<std::optional<double>> lerM;    // effective LE radius, per station
};

// Flight point. mach: dimensionless. alpha/beta: degrees at the public
// oracle boundary. reynoldsPerM: 1/m. roughnessM: absolute metres.
struct FlightCondition {
    double mach = 0.0;
    double reynoldsPerM = 0.0;
    std::string boundaryLayer = "TURB";  // "NATURAL" or "TURB"
    double roughnessM = 0.0;
};

// Aerodynamic reference quantities, SI units. Not derived silently.
struct ReferenceCondition {
    double areaM2 = 0.0;
    double lengthM = 0.0;
    double xcgM = 0.0;  // moment reference station, from nose, positive aft
};

// One-BODY + one-FINSET static-longitudinal request with no aero inputs.
// beta_deg, control_deflection_deg, and fin_cant_deg must all be zero
// (checked by validity — see Validity.hpp); the frozen contract does not
// support nonzero values for any of the three.
struct DatcomEquivalentRequest {
    BodyGeometry body;
    FinGeometry fin;
    FlightCondition flight;
    ReferenceCondition reference;
    double alphaDeg = 0.0;
    double betaDeg = 0.0;
    double controlDeflectionDeg = 0.0;
    double finCantDeg = 0.0;
};

// Diagnostic result. Mirrors SingleFinsetStaticResult field-for-field.
// Populated by the (not yet implemented, as of this session) numerical
// solver; the fields exist now so the validity/orchestrator boundary is
// typed correctly ahead of the physics port.
struct DatcomEquivalentResult {
    double finalCa = 0.0;
    double finalCn = 0.0;
    double finalCm = 0.0;

    double bodyCa = 0.0;
    double bodyCn = 0.0;
    double bodyCm = 0.0;

    double isolatedFinCa = 0.0;
    double isolatedFinCna = 0.0;
    double isolatedFinCn = 0.0;
    double isolatedFinCm = 0.0;
    std::optional<double> isolatedFinCp;

    double kwBase = 0.0;
    double kw = 0.0;
    double kb = 0.0;
    double kkw = 0.0;
    double kkb = 0.0;
    double bdwk = 0.0;

    std::vector<double> equivalentPanelAnglesDeg;
    std::vector<double> panelCn;

    double finInBodyCn = 0.0;
    double finInBodyCa = 0.0;
    double carryoverCn = 0.0;
    double carryoverCm = 0.0;

    std::string activeBodyMethod;
    bool bodyFallback = false;

    std::string validity;
    std::vector<std::string> warnings;
};

}  // namespace datcom_equivalent
