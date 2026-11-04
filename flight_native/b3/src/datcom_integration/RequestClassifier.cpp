#include "datcom_integration/RequestClassifier.hpp"

#include <algorithm>
#include <cmath>

namespace RoketSiM::DatcomIntegration {
namespace {

void addUnique(std::vector<CapabilityId>& values, CapabilityId value) {
    if (std::find(values.begin(), values.end(), value) == values.end()) values.push_back(value);
}

void error(std::vector<Diagnostic>& values, std::string id, std::string message,
           DiagnosticReason reason, std::optional<CapabilityId> capability = std::nullopt) {
    values.push_back({std::move(id), DiagnosticSeverity::Error, capability,
        std::move(message), reason});
}

void extrapolationWarning(std::vector<Diagnostic>& values, std::string message) {
    values.push_back({"DATCOM_TANGENT_OGIVE_TABLE_EXTRAPOLATION",
        DiagnosticSeverity::Warning, CapabilityId::BodyTangentOgive,
        std::move(message), DiagnosticReason::ValidityLimited,
        std::string("tangent_ogive_capability_closure"), std::nullopt});
}

CapabilityId bodyCapability(BodyFamily family) {
    switch (family) {
    case BodyFamily::SharpCone: return CapabilityId::BodySharpCone;
    case BodyFamily::TangentOgive: return CapabilityId::BodyTangentOgive;
    case BodyFamily::Parabolic: return CapabilityId::BodyParabolic;
    case BodyFamily::Power: return CapabilityId::BodyPower;
    case BodyFamily::Haack: return CapabilityId::BodyHaack;
    case BodyFamily::VonKarman: return CapabilityId::BodyVonKarman;
    case BodyFamily::SecantOgive: return CapabilityId::SecantOgiveNamed;
    case BodyFamily::Elliptical: return CapabilityId::BodyEllipsoid;
    case BodyFamily::Option2: return CapabilityId::BodyOption2Coordinates;
    case BodyFamily::Unknown: return CapabilityId::BodyUnknownNoseFamily;
    case BodyFamily::Missing: return CapabilityId::BodyOption2Coordinates;
    }
    return CapabilityId::BodyOption2Coordinates;
}

std::string bodyFamilyDisplayName(BodyFamily family) {
    switch (family) {
    case BodyFamily::SharpCone: return "Sharp Cone";
    case BodyFamily::TangentOgive: return "Tangent Ogive";
    case BodyFamily::Parabolic: return "Parabolic";
    case BodyFamily::Power: return "Power Series";
    case BodyFamily::Haack: return "Haack";
    case BodyFamily::VonKarman: return "Von Karman";
    case BodyFamily::SecantOgive: return "Secant Ogive";
    case BodyFamily::Elliptical: return "Ellipsoid";
    case BodyFamily::Option2: return "Option 2 Coordinates";
    case BodyFamily::Missing: return "Missing";
    case BodyFamily::Unknown: return "Unknown";
    }
    return "Unknown";
}

}  // namespace

ClassifiedRequest classifyRequest(MappedRequest mapped) {
    ClassifiedRequest out;
    out.diagnostics = mapped.diagnostics;
    out.mapped = std::move(mapped);
    auto& requested = out.requestedCapabilities;
    auto& observed = out.observedCapabilities;

    if (out.mapped.body.family == BodyFamily::Missing) {
        error(out.diagnostics, "DATCOM_BODY_REQUIRED", "A primary BODY nose is required.",
              DiagnosticReason::NotRepresentable);
    } else {
        addUnique(requested, out.mapped.body.family == BodyFamily::TangentOgive
            && out.mapped.condition.mach > 1.2
            ? CapabilityId::BodyTangentOgiveSupersonic
            : bodyCapability(out.mapped.body.family));
    }
    if (out.mapped.body.family == BodyFamily::Unknown) {
        error(out.diagnostics, "DATCOM_BODY_FAMILY_NOT_REPRESENTABLE",
              "The BODY family is preserved but has no deterministic DATCOM mapping.",
              DiagnosticReason::NotRepresentable, CapabilityId::BodyUnknownNoseFamily);
    }
    if (out.mapped.body.family != BodyFamily::Missing
        && out.mapped.body.family != BodyFamily::SharpCone
        && out.mapped.body.family != BodyFamily::TangentOgive
        && out.mapped.body.family != BodyFamily::Unknown) {
        error(out.diagnostics, "UNSUPPORTED_NOSE_FAMILY",
              "DATCOM Equivalent supports sharp conical and accepted SUBTRANS tangent-ogive noses. Current nose: "
              + bodyFamilyDisplayName(out.mapped.body.family) + ".",
              DiagnosticReason::NotImplemented, bodyCapability(out.mapped.body.family));
    }
    const auto parameter = out.mapped.body.shapeParameter;
    const auto family = out.mapped.body.family;
    const bool parameterized = family == BodyFamily::TangentOgive
        || family == BodyFamily::SecantOgive || family == BodyFamily::Parabolic
        || family == BodyFamily::Power || family == BodyFamily::Haack
        || family == BodyFamily::VonKarman;
    bool validParameter = !parameterized || (parameter && std::isfinite(*parameter));
    if (validParameter && parameterized) {
        const double p = *parameter;
        if (family == BodyFamily::TangentOgive) validParameter = std::abs(p - 1.0) <= 1e-12;
        else if (family == BodyFamily::SecantOgive) validParameter = p >= 0.0 && p < 1.0;
        else if (family == BodyFamily::Haack) validParameter = p > 0.0 && p <= 1.0 / 3.0;
        else if (family == BodyFamily::VonKarman) validParameter = std::abs(p) <= 1e-12;
        else validParameter = p >= 0.0 && p <= 1.0;
    }
    if (parameterized && (!validParameter
        || out.mapped.body.shapeParameterSource == "EXPLICIT_INVALID")) {
        Diagnostic diagnostic{"DATCOM_NOSE_SHAPE_PARAMETER_INVALID", DiagnosticSeverity::Error,
            bodyCapability(family), "The preserved nose shape parameter is missing, non-finite, or outside the source-family range.",
            DiagnosticReason::InvalidRequest};
        diagnostic.sourceComponentId = out.mapped.body.sourceComponentId;
        out.diagnostics.push_back(std::move(diagnostic));
    }
    if (out.mapped.body.sourceParameters.value("is_flipped", false)) {
        error(out.diagnostics, "UNSUPPORTED_NOSE_FAMILY",
              "The compatibility operation requires a nose-forward BODY.",
              DiagnosticReason::NotImplemented, bodyCapability(out.mapped.body.family));
    }
    for (const char* key : {"shoulder_length", "shoulder_diameter", "aft_shoulder_length", "aft_shoulder_diameter"}) {
        const auto found = out.mapped.body.sourceParameters.find(key);
        if ((out.mapped.body.family == BodyFamily::SharpCone
             || out.mapped.body.family == BodyFamily::TangentOgive)
            && found != out.mapped.body.sourceParameters.end() && found->is_number()
            && std::abs(found->get<double>()) > 1e-12) {
            error(out.diagnostics, "UNSUPPORTED_NOSE_FAMILY", "Nose shoulders are not supported.",
                  DiagnosticReason::NotImplemented, bodyCapability(out.mapped.body.family));
            break;
        }
    }

    int cylinders = 0;
    for (const auto& section : out.mapped.bodySections) {
        if (section.family == "CONSTANT_CYLINDER") {
            ++cylinders;
        } else {
            const bool boattail = section.aftDiameterM < section.foreDiameterM;
            const bool flare = section.aftDiameterM > section.foreDiameterM;
            if (section.family == "OGIVAL") {
                if (boattail) addUnique(requested, CapabilityId::BodyBoattailOgival);
                if (flare) addUnique(requested, CapabilityId::BodyFlareOgival);
            } else {
                if (boattail) addUnique(requested, CapabilityId::BodyBoattailConical);
                if (flare) addUnique(requested, CapabilityId::BodyFlareConical);
            }
        }
    }

    if (out.mapped.finsets.size() == 1) addUnique(requested, CapabilityId::FinsetSingle);
    if (out.mapped.finsets.size() > 1) {
        addUnique(requested, CapabilityId::FinsetMultiple);
        addUnique(requested, CapabilityId::FinFinInterference);
    }
    if (!out.mapped.finsets.empty()) {
        addUnique(requested, CapabilityId::FinNpanelVariants);
        addUnique(requested, CapabilityId::FinCrossSection);
        addUnique(requested, CapabilityId::FinAxial);
        addUnique(requested, CapabilityId::FinNormalForce);
        addUnique(requested, CapabilityId::FinMomentCp);
        addUnique(requested, CapabilityId::FinOrientation);
        addUnique(requested, CapabilityId::BodyFinInterference);
    }
    for (const auto& fin : out.mapped.finsets) {
        if (fin.mappedProfile == "ARC" || fin.mappedProfile == "arc") addUnique(requested, CapabilityId::ArcType2Fin);
        if (fin.sectionSupport == FinSectionSupport::NotImplemented) {
            const bool incompleteHex = fin.sectionFamily == FinSectionFamily::Hex;
            Diagnostic diagnostic{incompleteHex ? "DATCOM_FIN_SECTION_GEOMETRY_INCOMPLETE"
                                                : "DATCOM_FIN_SECTION_NOT_IMPLEMENTED",
                DiagnosticSeverity::Error,
                CapabilityId::FinCrossSection,
                incompleteHex
                    ? "FINSET '" + fin.sourceComponentId + "' generic HEX section is preserved, but canonical state does not define LMAX/LFLAT/LER; Diamond defaults are not inferred."
                    : "FINSET '" + fin.sourceComponentId + "' section " + fin.sourceProfile
                        + " is preserved; its DATCOM method is not implemented. Planform is checked separately.",
                DiagnosticReason::NotImplemented};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            out.diagnostics.push_back(std::move(diagnostic));
        }
        if (fin.sourceParameters.value("fillet_radius_m", 0.0) > 0.0) {
            Diagnostic diagnostic{"DATCOM_FIN_FILLET_OMITTED", DiagnosticSeverity::Warning,
                CapabilityId::FinCrossSection,
                "Fin fillet is preserved in the project but omitted from DATCOM aerodynamic geometry.",
                DiagnosticReason::ExplicitExclusion};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            out.diagnostics.push_back(std::move(diagnostic));
        }
        if (!std::isfinite(fin.thicknessM) || !(fin.thicknessM > 0.0)) {
            Diagnostic diagnostic{"INVALID_FIN_THICKNESS", DiagnosticSeverity::Error,
                CapabilityId::FinCrossSection, "FINSET physical thickness must be finite and positive.",
                DiagnosticReason::InvalidRequest};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            out.diagnostics.push_back(std::move(diagnostic));
        }
        if (!fin.exactPlanform) {
            Diagnostic diagnostic{"DATCOM_FIN_PLANFORM_NOT_REPRESENTABLE", DiagnosticSeverity::Error,
                CapabilityId::FinsetSingle, "FINSET outline is not an exact supported station planform.",
                DiagnosticReason::NotRepresentable};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            out.diagnostics.push_back(std::move(diagnostic));
        }
        if (fin.sectionSupport == FinSectionSupport::Exact && fin.exactPlanform
            && std::isfinite(fin.thicknessM) && fin.thicknessM > 0.0) {
            Diagnostic diagnostic{"DATCOM_FIN_SECTION_EXACT", DiagnosticSeverity::Info,
                CapabilityId::FinCrossSection,
                "Diamond admitted as sharp symmetric provider HEX: LMAXU/LMAXL=0.5, LFLATU/LFLATL=0, LER=0, and local t/(2*c_i) ordinates.",
                DiagnosticReason::ValidityLimited};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            out.diagnostics.push_back(std::move(diagnostic));
        }
        if (fin.exactPlanform && fin.exactPlanform->tipChordM == 0.0
            && fin.exactPlanform->source == FinPlanformSource::FreeformSourceOutline
            && fin.sectionSupport == FinSectionSupport::Exact) {
            Diagnostic diagnostic{"DATCOM_FIN_STATION_CHORD_INVALID", DiagnosticSeverity::Error,
                CapabilityId::FinCrossSection,
                "Exact planform has zero tip chord; the existing HEX t/(2*c_i) contract requires positive station chords.",
                DiagnosticReason::InvalidRequest};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            out.diagnostics.push_back(std::move(diagnostic));
        }
        if (fin.exactPlanform
            && fin.exactPlanform->source == FinPlanformSource::FreeformSourceOutline) {
            Diagnostic diagnostic{"DATCOM_FREEFORM_PLANFORM_EXACT", DiagnosticSeverity::Info,
                CapabilityId::FinsetSingle,
                "Source outline proves exact two-station geometry; execution remains subject to section, panel, validity and single-FINSET admission.",
                DiagnosticReason::ValidityLimited};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            out.diagnostics.push_back(std::move(diagnostic));
        }
        if (fin.panelCount != 3 && fin.panelCount != 4 && fin.panelCount != 6) {
            error(out.diagnostics, "UNSUPPORTED_PANEL_COUNT",
                  "The compatibility operation supports FINSET panel counts 3, 4, or 6.",
                  DiagnosticReason::NotImplemented, CapabilityId::FinNpanelVariants);
        }
        if (std::abs(fin.cantDeg) > 0.0) addUnique(requested, CapabilityId::ControlDeflct);
    }

    for (const auto& feature : out.mapped.optionalFeatures) {
        if (feature.type == "Launch Lug" || feature.type == "Rail Button") {
            addUnique(observed, CapabilityId::Protub);
            out.diagnostics.push_back({"EXCLUDED_EXTERNAL_PROTUBERANCE", DiagnosticSeverity::Warning,
                CapabilityId::Protub, feature.type
                + " is present but excluded from DATCOM Equivalent aerodynamic coefficients.",
                DiagnosticReason::ExplicitExclusion, std::string("targeted_coordinate_body_re"),
                std::string("BUG-016")});
        }
        if (feature.type == "Inlet") addUnique(requested, CapabilityId::Inlet);
    }
    for (const auto& feature : out.mapped.unsupportedComponents) {
        error(out.diagnostics, "DATCOM_COMPONENT_NOT_REPRESENTABLE",
              "Component '" + feature.sourceComponentId + "' (" + feature.type
              + ") is preserved but not representable by the compatibility operation.",
              DiagnosticReason::NotRepresentable);
    }

    if (std::abs(out.mapped.condition.betaDeg) > 0.0) addUnique(requested, CapabilityId::BetaOrientation);
    if (std::abs(out.mapped.condition.finCantDeg) > 0.0
        || std::abs(out.mapped.condition.controlDeflectionDeg) > 0.0) {
        addUnique(requested, CapabilityId::ControlDeflct);
    }
    switch (out.mapped.condition.analysisMode) {
    case AnalysisMode::StaticLongitudinal: break;
    case AnalysisMode::StaticDerivatives: addUnique(requested, CapabilityId::StaticDerivatives); break;
    case AnalysisMode::DynamicDerivatives: addUnique(requested, CapabilityId::DynamicDerivatives); break;
    case AnalysisMode::MagnusSpin: addUnique(requested, CapabilityId::MagnusSpinDerivatives); break;
    case AnalysisMode::TrimPitch: addUnique(requested, CapabilityId::TrimPitch); break;
    case AnalysisMode::HypersBodyOnly:
        addUnique(requested, out.mapped.finsets.empty() ? CapabilityId::HypersBodyOnly : CapabilityId::HyperFin);
        break;
    case AnalysisMode::BasePlume: addUnique(requested, CapabilityId::BasePlumeChapmanKorst); break;
    case AnalysisMode::Inlet: addUnique(requested, CapabilityId::Inlet); break;
    }

    const auto& condition = out.mapped.condition;
    if (out.mapped.body.family == BodyFamily::TangentOgive && condition.mach < 0.8
        && condition.mach >= 0.2) {
        out.diagnostics.push_back({"DATCOM_TANGENT_OGIVE_POTENTIAL_MACH_FLOOR",
            DiagnosticSeverity::Warning, CapabilityId::BodyTangentOgive,
            "Below Mach 0.80, tangent-ogive potential CN/CM uses the Mach 0.80 correlation; viscous and axial terms still use the requested Mach.",
            DiagnosticReason::ValidityLimited, std::string("tangent_ogive_capability_closure"),
            std::nullopt});
    }
    // BDCNAN/BDXCPN use LNTRP outside their finite tabulated axes.  This is
    // intentionally a non-blocking applicability diagnostic: source behavior
    // is preserved, but callers must not mistake an extrapolated value for an
    // interpolated correlation result.  Tangent-ogive theoretical fineness is
    // LNOSE/DNOSE (the geometry transform reduces algebraically to that ratio).
    if (out.mapped.body.family == BodyFamily::TangentOgive
        && out.mapped.body.diameterM > 0.0) {
        const double fnose = out.mapped.body.lengthM / out.mapped.body.diameterM;
        const double fcenter = out.mapped.bodySections.empty() ? 0.0
            : out.mapped.bodySections.front().lengthM / out.mapped.body.diameterM;
        if (fnose > 3.5) {
            extrapolationWarning(out.diagnostics,
                "FNOSE=" + std::to_string(fnose)
                + " exceeds the last real BDXCPN tangent-ogive anchor (3.50); "
                  "BDXCPN has no fineness-10 anchor and extrapolates the 2.50-to-3.50 segment.");
        }
        // TFA2 owns OGIV05/OGIV15; TFA3 owns OGIV25/OGIV35.  Between
        // FNOSE=1.50 and 2.50 both families participate, so retain both
        // independent limits instead of reporting a misleading single bound.
        const bool usesTfa2 = fnose <= 2.5;
        const bool usesTfa3 = fnose >= 1.5;
        if (usesTfa2 && fcenter > 20.0) {
            extrapolationWarning(out.diagnostics,
                "FCENTER=" + std::to_string(fcenter)
                + " exceeds the last real TFA2 anchor (20.00) used by BDCNAN/BDXCPN tangent-ogive tables.");
        }
        if (usesTfa3 && fcenter > 18.0) {
            extrapolationWarning(out.diagnostics,
                "FCENTER=" + std::to_string(fcenter)
                + " exceeds the last real TFA3 anchor (18.00) used by BDCNAN/BDXCPN tangent-ogive tables.");
        }
    }
    if (condition.mach < 0.2 || condition.mach > 5.0)
        error(out.diagnostics, "MACH_OUT_OF_RANGE", "Mach must be within [0.2, 5.0].", DiagnosticReason::InvalidRequest);
    if (condition.alphaDeg < -10.0 || condition.alphaDeg > 10.0)
        error(out.diagnostics, "ALPHA_OUT_OF_RANGE", "Alpha must be within [-10, 10] degrees.", DiagnosticReason::InvalidRequest);
    if (!(condition.reynoldsPerM > 0.0))
        error(out.diagnostics, "INVALID_REYNOLDS", "Unit Reynolds must be positive in 1/m.", DiagnosticReason::InvalidRequest);
    if (condition.roughnessM < 0.0)
        error(out.diagnostics, "DATCOM_INVALID_ROUGHNESS", "Roughness must be nonnegative metres.", DiagnosticReason::InvalidRequest);
    if (condition.boundaryLayer != "NATURAL" && condition.boundaryLayer != "TURB")
        error(out.diagnostics, "UNSUPPORTED_BOUNDARY_LAYER_MODE", "Boundary layer must be NATURAL or TURB.", DiagnosticReason::InvalidRequest);
    if (std::abs(condition.altitudeM) > 1.0e-9)
        error(out.diagnostics, "UNSUPPORTED_CONDITION", "DATCOM Equivalent static analysis does not consume altitude; use 0 m.", DiagnosticReason::InvalidRequest);
    if (!(out.mapped.reference.areaM2 > 0.0))
        error(out.diagnostics, "DATCOM_INVALID_REFERENCE_AREA", "Reference area must be positive m^2.", DiagnosticReason::InvalidRequest);
    if (!(out.mapped.reference.lengthM > 0.0))
        error(out.diagnostics, "DATCOM_INVALID_REFERENCE_LENGTH", "Reference length must be positive m.", DiagnosticReason::InvalidRequest);

    if (out.mapped.condition.analysisMode == AnalysisMode::StaticLongitudinal) {
        if (out.mapped.bodySections.size() != 1)
            error(out.diagnostics, "UNSUPPORTED_BODY_GEOMETRY",
                  "The compatibility operation supports exactly one constant-diameter body tube; transition/boattail geometry is not executable.",
                  DiagnosticReason::NotImplemented);
        if (cylinders != 1)
            error(out.diagnostics, "DATCOM_COMPATIBILITY_BODY_SECTION_COUNT",
                  "The compatibility operation requires exactly one constant-diameter body tube.",
                  DiagnosticReason::NotImplemented);
        if (out.mapped.finsets.size() != 1)
            error(out.diagnostics, out.mapped.finsets.size() > 1 ? "MULTIPLE_FINSETS" : "UNSUPPORTED_FIN_PROFILE",
                  "The compatibility operation requires exactly one FINSET.", DiagnosticReason::NotImplemented);
        if ((out.mapped.body.family == BodyFamily::SharpCone
             || out.mapped.body.family == BodyFamily::TangentOgive) && cylinders == 1) {
            const auto& body = out.mapped.bodySections.front();
            if (std::abs(out.mapped.body.originM) > 1e-9
                || std::abs(body.originM - out.mapped.body.lengthM) > 1e-9
                || std::abs(body.foreDiameterM - out.mapped.body.diameterM) > 1e-9) {
                error(out.diagnostics, "DATCOM_COMPATIBILITY_BODY_DISCONTINUITY",
                      "Compatibility geometry must be a nose-origin sharp cone or tangent ogive contiguous with an equal-diameter cylinder.",
                      DiagnosticReason::NotImplemented, bodyCapability(out.mapped.body.family));
            }
            if (out.mapped.body.family == BodyFamily::TangentOgive
                && out.mapped.body.lengthM < out.mapped.body.diameterM / 2.0) {
                error(out.diagnostics, "DATCOM_TANGENT_OGIVE_TOO_SHORT",
                      "Tangent-ogive geometry requires nose length greater than or equal to half the base diameter.",
                      DiagnosticReason::InvalidRequest, CapabilityId::BodyTangentOgive);
            }
            if (out.mapped.body.diameterM < 0.05 || out.mapped.body.diameterM > 0.4)
                error(out.diagnostics, "DIAMETER_OUT_OF_RANGE",
                      "Body diameter must be within [0.05, 0.4] m.", DiagnosticReason::InvalidRequest,
                      bodyCapability(out.mapped.body.family));
        }
    }

    return out;
}

}  // namespace RoketSiM::DatcomIntegration
