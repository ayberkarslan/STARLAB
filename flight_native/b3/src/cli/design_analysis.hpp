#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nlohmann/json.hpp"
#include "aero/Barrowman.hpp"
#include "aero/extensions/AeroExtensionPipeline.hpp"
#include "barrowman_drag.hpp"
#include "freeform_planform.hpp"
#include "aero/FinSetBarrowman.hpp"
#include "components/MotorCluster.hpp"
#include "malzeme/VeriTabanimalzeme.h"

namespace design {

using json = nlohmann::json;
constexpr double kPi = 3.14159265358979323846;

inline double number(const json& object, const char* key, double fallback = 0.0) {
    if (!object.is_object() || !object.contains(key) || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<double>();
}

inline std::string text(
    const json& object,
    const char* key,
    const std::string& fallback = ""
) {
    if (!object.is_object() || !object.contains(key) || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<std::string>();
}

inline bool enabled(const json& overrides, const char* key) {
    return overrides.is_object() && overrides.contains(key)
        && overrides[key].is_object()
        && overrides[key].contains("enabled")
        && overrides[key]["enabled"].get<bool>();
}

inline RoketSiM::AeroExtensions::AeroExtensionMode aeroExtensionMode(
    const std::string& value
) {
    using namespace RoketSiM::AeroExtensions;
    if (value == "DISABLED" || value == "disabled") {
        return AeroExtensionMode::Disabled;
    }
    if (value == "DIAGNOSTIC_ONLY" || value == "diagnostic_only") {
        return AeroExtensionMode::DiagnosticOnly;
    }
    if (value == "ACTIVE" || value == "active") {
        return AeroExtensionMode::Active;
    }
    throw std::runtime_error("Unknown aero extension mode: " + value);
}

inline RoketSiM::AeroExtensions::AeroExtensionConfiguration
aeroExtensionConfiguration(
    const json& input
) {
    using namespace RoketSiM::AeroExtensions;
    AeroExtensionConfiguration configuration;
    if (!input.contains("aero_extensions")
        || !input["aero_extensions"].is_object()) {
        return configuration;
    }
    const json& root = input["aero_extensions"];
    const std::string failurePolicy = text(root, "failure_policy", "SAFE_FALLBACK");
    if (failurePolicy == "STRICT" || failurePolicy == "strict") {
        configuration.failurePolicy = AeroExtensionFailurePolicy::Strict;
    } else if (failurePolicy != "SAFE_FALLBACK"
               && failurePolicy != "safe_fallback"
               && failurePolicy != "safe") {
        throw std::runtime_error(
            "Unknown aero extension failure policy: " + failurePolicy);
    }
    const json& selected = root.contains("extensions")
            && root["extensions"].is_object()
        ? root["extensions"]
        : (root.contains("switches") && root["switches"].is_object()
            ? root["switches"] : root);
    for (auto item = selected.begin(); item != selected.end(); ++item) {
        if (item.key() == "failure_policy" || item.key() == "schema_version") {
            continue;
        }
        AeroExtensionSelection selection;
        if (item.value().is_boolean()) {
            selection.mode = item.value().get<bool>()
                ? AeroExtensionMode::Active : AeroExtensionMode::Disabled;
        } else if (item.value().is_object()) {
            const json& value = item.value();
            if (value.contains("mode") && value["mode"].is_string()) {
                selection.mode = aeroExtensionMode(value["mode"].get<std::string>());
            } else if (value.contains("enabled") && value["enabled"].is_boolean()) {
                selection.mode = value["enabled"].get<bool>()
                    ? AeroExtensionMode::Active : AeroExtensionMode::Disabled;
            }
            selection.model = text(value, "model");
            const std::string applicationMode = text(
                value, "application_mode", "DELTA");
            if (applicationMode == "COMPONENT_REPLACEMENT"
                || applicationMode == "component_replacement") {
                selection.applicationMode =
                    AeroExtensionApplicationMode::ComponentReplacement;
            } else if (applicationMode != "DELTA" && applicationMode != "delta") {
                throw std::runtime_error(
                    "Unknown aero extension application mode: " + applicationMode);
            }
            selection.allowComponentReplacement = value.value(
                "allow_component_replacement", false);
            if (value.contains("parameters") && value["parameters"].is_object()) {
                for (auto parameter = value["parameters"].begin();
                     parameter != value["parameters"].end(); ++parameter) {
                    if (!parameter.value().is_number()) {
                        throw std::runtime_error(
                            "Aero extension parameters must be numeric: "
                            + item.key() + "." + parameter.key());
                    }
                    selection.parameters[parameter.key()] =
                        parameter.value().get<double>();
                }
            }
        } else {
            throw std::runtime_error(
                "Aero extension selection must be boolean or object: "
                + item.key());
        }
        configuration.extensions[item.key()] = std::move(selection);
    }
    return configuration;
}

inline bool anyAeroExtensionEnabled(
    const RoketSiM::AeroExtensions::AeroExtensionConfiguration& configuration
) {
    return std::any_of(configuration.extensions.begin(),
        configuration.extensions.end(), [](const auto& item) {
            return item.second.mode
                != RoketSiM::AeroExtensions::AeroExtensionMode::Disabled;
        });
}

inline json aeroExtensionDeltaJson(
    const RoketSiM::AeroExtensions::AeroExtensionDelta& delta
) {
    return {
        {"force_n", {delta.forceN.x, delta.forceN.y, delta.forceN.z}},
        {"moment_nm", {delta.momentNm.x, delta.momentNm.y, delta.momentNm.z}},
        {"coefficients", {
            {"ca", delta.coefficients.axialForce},
            {"cy", delta.coefficients.sideForce},
            {"cn", delta.coefficients.normalForce},
            {"cl", delta.coefficients.rollMoment},
            {"cm", delta.coefficients.pitchMoment},
            {"cn_yaw", delta.coefficients.yawMoment},
            {"cd", delta.coefficients.drag},
            {"cna_per_rad", delta.coefficients.normalForceSlopePerRad},
            {"cna_cp_m_per_rad",
             delta.coefficients.normalForceSlopeMomentMPerRad},
        }},
    };
}

inline json aeroExtensionEvaluationJson(
    const RoketSiM::AeroExtensions::AeroExtensionEvaluation& evaluation
) {
    using namespace RoketSiM::AeroExtensions;
    json records = json::array();
    for (const auto& record : evaluation.records) {
        records.push_back({
            {"id", record.metadata.id},
            {"display_name", record.metadata.displayName},
            {"version", record.metadata.version},
            {"family", record.metadata.family},
            {"source_methods", record.metadata.sourceMethods},
            {"source_references", record.metadata.sourceReferences},
            {"port_priority", record.metadata.portPriority},
            {"available", record.metadata.available},
            {"enabled", record.enabled},
            {"mode", aeroExtensionModeName(record.mode)},
            {"configured_model", record.configuredModel},
            {"configuration_parameters", record.configuredParameters},
            {"requested_application_mode", aeroExtensionApplicationModeName(
                record.requestedApplicationMode)},
            {"status", aeroExtensionStatusName(record.status)},
            {"validity", aeroExtensionValidityName(record.validity)},
            {"lifecycle", aeroExtensionLifecycleName(record.metadata.lifecycle)},
            {"application_mode", aeroExtensionApplicationModeName(
                record.metadata.applicationMode)},
            {"applicability", record.metadata.applicability},
            {"supported_flow_regimes", record.metadata.supportedFlowRegimes},
            {"dependencies", record.metadata.dependencies},
            {"overlaps_with", record.metadata.overlapsWith},
            {"limitations", record.metadata.limitations},
            {"delta", aeroExtensionDeltaJson(record.delta)},
            {"diagnostics", record.diagnostics},
            {"diagnostic_values", record.diagnosticValues},
        });
    }
    const auto& baseline = evaluation.baseline.coefficients;
    const auto& final = evaluation.finalResult.coefficients;
    return {
        {"schema_version", 2},
        {"baseline_provider", "internal"},
        {"failure_policy", aeroExtensionFailurePolicyName(
            evaluation.failurePolicy)},
        {"result_classification", evaluation.augmented
            ? "AUGMENTED" : "BASELINE_ONLY"},
        {"baseline", {
            {"cd", baseline.drag},
            {"cna_per_rad", baseline.normalForceSlopePerRad},
            {"cna_cp_m_per_rad", baseline.normalForceSlopeMomentMPerRad},
        }},
        {"final", {
            {"cd", final.drag},
            {"cna_per_rad", final.normalForceSlopePerRad},
            {"cna_cp_m_per_rad", final.normalForceSlopeMomentMPerRad},
        }},
        {"extensions", records},
        {"unknown_enabled_extension_ids", evaluation.unknownEnabledExtensionIds},
    };
}

inline bool isExternallyExposedForAero(const json& component) {
    // Missing is backward compatible for pre-aero_exposure project files.
    // Current Python project migration always writes the field.
    return component.value("aero_active", true)
        && text(component, "aero_exposure", "external") != "internal";
}

inline std::string canonicalMaterialName(const std::string& material) {
    const std::size_t separator = material.find(':');
    const std::string unqualified = separator == std::string::npos
        ? material : material.substr(separator + 1);
    if (unqualified == "aluminum_6061" || unqualified == "Aluminum 6061") {
        return "Alüminyum";
    }
    if (unqualified == "steel_1018" || unqualified == "Steel 1018") {
        return "Çelik";
    }
    if (unqualified == "acrylic" || unqualified == "Acrylic") {
        return "Akrilik";
    }
    return unqualified;
}

inline double densityFor(
    const std::string& material,
    Malzeme::Type type = Malzeme::Type::YIGIN
) {
    const std::string canonical = canonicalMaterialName(material);
    if (const Malzeme* found =
        VeriTabanimalzeme::bulMateryal(type, canonical)) {
        return found->alYogunluk();
    }
    const auto& database = VeriTabanimalzeme::alVeritabani(type);
    if (!database.empty()) return database.front().alYogunluk();
    return type == Malzeme::Type::YIGIN ? 2700.0 : 0.0;
}
struct ComponentResult {
    std::string id;
    std::string type;
    double position = 0.0;
    double radialX = 0.0;
    double radialY = 0.0;
    double angleDeg = 0.0;
    double orientationDeg = 0.0;
    bool physicallyInstanced = false;
    double length = 0.0;
    double volume = 0.0;
    double mass = 0.0;
    double localCg = 0.0;
    // Offsets of physical instances from this component definition's axial
    // reference position.  ``localCg`` remains the centroid of one physical
    // instance; the aggregate component CG is resolved from this vector.
    std::vector<double> axialInstanceOffsets{0.0};
    double iTransverse = 0.0;
    double iAxial = 0.0;
    Eigen::Matrix3d inertiaLocal = Eigen::Matrix3d::Zero();
    double cna = 0.0;
    double cp = 0.0;
    bool aerodynamic = false;
    bool fin = false;
    double finArea = 0.0;
    double finSpan = 0.0;
    double finCosGamma = 1.0;
    double finAspectRatio = 0.0;
    double finMacLength = 0.0;
    double finMacLead = 0.0;
    double finBodyRadius = 0.0;
    int finCount = 0;
    int finInterferenceCount = 0;
    // TubeFinSetCalc uses Ribner CNa at every Mach, but its CP follows a
    // separate legacy OpenRocket 24.12 branch.  Keep the aspect ratio needed
    // for its supersonic relation with the component rather than deriving it
    // from a fixture or a global reference diameter.
    bool tubeFin = false;
    double tubeFinAspectRatio = 0.0;
    std::string noseShape;
    double noseBaseRadius = 0.0;
    double noseShapeParameter = 0.0;
    bool motor = false;
    double motorDryMass = 0.0;
    double motorPropellantMass = 0.0;
    std::string motorDesignation;
    std::string motorManufacturer;
    std::string motorCaseInfo;
    std::string motorPropellantInfo;
    std::string motorDelays;
    bool motorSparky = false;
    bool motorAvailable = true;
    MotorClusterConfiguration motorCluster;
    std::vector<Eigen::Vector2d> motorClusterOffsets;
};

inline double aggregateLocalCg(const ComponentResult& component) {
    if (component.axialInstanceOffsets.empty()) return component.localCg;
    double offsetSum = 0.0;
    for (const double offset : component.axialInstanceOffsets) {
        offsetSum += offset;
    }
    return component.localCg + offsetSum
        / static_cast<double>(component.axialInstanceOffsets.size());
}

inline double finInterferenceFactor(int count) {
    return FinSetBarrowman::finInterferenceFactor(count);
}

inline double finCnaAtMach(const ComponentResult& fin, double mach,
                           double referenceArea, double angleOfAttack = 0.0) {
    if (!fin.fin || fin.finArea <= 1e-12 || fin.finSpan <= 1e-12) return 0.0;
    double cna = FinSetBarrowman::aggregateCna(
        fin.finArea, fin.finSpan, fin.finCosGamma,
        mach, angleOfAttack, referenceArea,
        fin.finCount, fin.finBodyRadius, fin.finInterferenceCount);
    // OpenRocket's FinSetCalc projects a singleton fin with
    // sin²(theta - transform.xRotation).  The component-analysis condition
    // uses theta=0; sets with two or more fins already include their own ring
    // distribution in aggregateCna.
    if (fin.finCount == 1) {
        const double orientation = fin.orientationDeg * kPi / 180.0;
        cna *= std::pow(std::sin(orientation), 2.0);
    }
    return cna;
}

inline double finCpFraction(const ComponentResult& fin, double mach) {
    return FinSetBarrowman::cpFraction(fin.finAspectRatio, mach);
}

inline double tubeFinCpFraction(double mach, double aspectRatio) {
    // Exact OpenRocket 24.12 TubeFinSetCalc.calculateCPPos behavior.  In
    // that release the interpolation coefficients are allocated but never
    // initialized, so the 0.5 < M < 2 polynomial evaluates to zero.  This is
    // compatibility behavior, not a fitted correction.
    if (mach <= 0.5) return 0.25;
    if (mach < 2.0) return 0.0;
    const double beta = std::sqrt(std::max(mach * mach - 1.0, 0.0));
    const double denominator = 2.0 * aspectRatio * beta - 1.0;
    return std::abs(denominator) > 1.0e-12
        ? (aspectRatio * beta - 0.67) / denominator : 0.0;
}
inline double noseRadius(
    const std::string& shape,
    double z,
    double length,
    double baseRadius,
    const json& params
) {
    if (length <= 0.0) return 0.0;
    const double x = std::clamp(z / length, 0.0, 1.0);
    if (shape == "Conical") return baseRadius * x;
    if (shape == "Ellipsoid") {
        return baseRadius * std::sqrt(std::max(0.0, 1.0 - (1.0 - x) * (1.0 - x)));
    }
    if (shape == "Parabolic") {
        const double k = number(params, "K", 0.5);
        return baseRadius * (2.0 * x - k * x * x) / std::max(2.0 - k, 1e-9);
    }
    if (shape == "Power") {
        return baseRadius * std::pow(x, number(params, "n", 0.5));
    }
    if (shape == "Haack") {
        const double theta = std::acos(std::clamp(1.0 - 2.0 * x, -1.0, 1.0));
        const double c = number(params, "C", 0.0);
        const double inside = std::max(
            theta - 0.5 * std::sin(2.0 * theta)
                + c * std::pow(std::sin(theta), 3.0),
            0.0
        );
        return baseRadius * std::sqrt(inside / kPi);
    }
    // Tangent ogive
    if (baseRadius <= 0.0) return 0.0;
    const double rho =
        (baseRadius * baseRadius + length * length) / (2.0 * baseRadius);
    return std::sqrt(std::max(
        rho * rho - (length - z) * (length - z),
        0.0
    )) + baseRadius - rho;
}

inline double tangentOgiveCpFromVolume(double length, double baseRadius) {
    if (length <= 0.0 || baseRadius <= 0.0) return 0.0;
    const double rho = (baseRadius * baseRadius + length * length)
        / (2.0 * baseRadius);
    constexpr int slices = 2000;
    const double dz = length / slices;
    double volume = 0.0;
    for (int i = 0; i < slices; ++i) {
        const double z = (i + 0.5) * dz;
        const double r = std::sqrt(std::max(
            rho * rho - (length - z) * (length - z), 0.0))
            + baseRadius - rho;
        volume += kPi * r * r * dz;
    }
    return length - volume / (kPi * baseRadius * baseRadius);
}

template <typename RadiusFunction>
inline void integrateShell(
    double length,
    double thickness,
    double density,
    RadiusFunction radius,
    ComponentResult& result
) {
    // Match OpenRocket SymmetricComponent.calculateProperties(): it divides
    // the profile into 128 conical frusta and converts a wall-normal thickness
    // into each frustum's radial inner profile.  A radial-only shell is
    // noticeably light for an ogive nose cone.
    constexpr int slices = 128;
    if (length <= 0.0) return;
    const double dz = length / slices;
    double firstMoment = 0.0;
    double transverseOrigin = 0.0;
    for (int i = 0; i < slices; ++i) {
        const double z0 = i * dz;
        const double z1 = (i + 1) * dz;
        const double ro0 = std::max(radius(z0), 0.0);
        const double ro1 = std::max(radius(z1), 0.0);
        const double radialWall = std::max(thickness, 0.0)
            * std::hypot(ro1 - ro0, dz) / dz;
        const double ri0 = std::max(ro0 - radialWall, 0.0);
        const double ri1 = std::max(ro1 - radialWall, 0.0);
        const auto frustumVolume = [&](double r0, double r1) {
            return kPi * dz * (r0 * r0 + r0 * r1 + r1 * r1) / 3.0;
        };
        const auto frustumCg = [&](double r0, double r1) {
            const double denominator = r0 * r0 + r0 * r1 + r1 * r1;
            return denominator > 1.0e-18 ? dz * (r0 * r0 + 2.0 * r0 * r1
                + 3.0 * r1 * r1) / (4.0 * denominator) : dz / 2.0;
        };
        const double outerVolume = frustumVolume(ro0, ro1);
        const double innerVolume = frustumVolume(ri0, ri1);
        const double dVolume = outerVolume - innerVolume;
        const double dm = density * dVolume;
        const double localCg = dVolume > 1.0e-18
            ? (outerVolume * frustumCg(ro0, ro1)
                - innerVolume * frustumCg(ri0, ri1)) / dVolume : dz / 2.0;
        const double z = z0 + localCg;
        result.volume += dVolume;
        result.mass += dm;
        firstMoment += dm * z;
        const double ro = (ro0 + ro1) / 2.0;
        const double ri = (ri0 + ri1) / 2.0;
        result.iAxial += 0.5 * dm * (ro * ro + ri * ri);
        transverseOrigin +=
            dm * (0.25 * (ro * ro + ri * ri) + z * z);
    }
    if (result.mass > 0.0) {
        result.localCg = firstMoment / result.mass;
        result.iTransverse =
            std::max(transverseOrigin - result.mass * result.localCg
                * result.localCg, 0.0);
    }
}

inline ComponentResult analyzeComponent(
    const json& component,
    double position,
    double referenceDiameter
) {
    ComponentResult result;
    result.id = text(component, "id");
    result.type = text(component, "type");
    const std::string subtype = text(component, "sub_type");
    const json params = component.contains("params")
        ? component["params"] : json::object();
    const json overrides = component.contains("overrides")
        ? component["overrides"] : json::object();
    const Malzeme::Type materialType = result.type == "Shock Cord"
        ? Malzeme::Type::CIZGI : Malzeme::Type::YIGIN;
    const double catalogDensity = densityFor(
        text(params, "material", result.type == "Shock Cord"
            ? "İp (Dayanıklı)" : "Alüminyum"), materialType);
    const double density = number(params, "source_bulk_density_kg_m3", -1.0)
        >= 0.0 ? number(params, "source_bulk_density_kg_m3") : catalogDensity;
    result.position = position;
    const std::string radialMode = text(component, "radial_mode", "Coaxial");
    const double radialOffset = number(component, "radial_offset_m", 0.0);
    result.angleDeg = number(component, "angle_deg", 0.0);
    result.orientationDeg = number(component, "physical_orientation_deg", result.angleDeg);
    result.physicallyInstanced = component.contains("physical_instance_index");
    double radialRadius = 0.0;
    if (radialMode == "Free Offset") {
        radialRadius = std::max(radialOffset, 0.0);
    } else if (radialMode == "On Parent Surface") {
        radialRadius = std::max(
            number(params, "body_radius", referenceDiameter / 2.0)
                + radialOffset,
            0.0);
    }
    const double radialAngle = result.angleDeg * kPi / 180.0;
    result.radialX = radialRadius * std::cos(radialAngle);
    result.radialY = radialRadius * std::sin(radialAngle);

    if (result.type == "Nose Cone") {
        result.length = number(params, "length", 0.3);
        const double radius = number(params, "diameter", referenceDiameter) / 2.0;
        result.noseShape = subtype;
        result.noseBaseRadius = radius;
        result.noseShapeParameter = number(params, "shape_parameter", 1.0);
        integrateShell(
            result.length,
            params.value("filled", false)
                ? radius : number(params, "thickness", 0.002),
            density,
            [&](double z) {
                return noseRadius(text(params, "mass_shape", subtype), z,
                    result.length, radius, params);
            },
            result
        );
        // Transition.calculateProperties() adds the aft shoulder outside the
        // nose-cone length and, if enabled, its closing cap.  It belongs to
        // the same physical component and therefore must participate in the
        // same global CG/inertia transform.
        const double shoulderLength = std::max(0.0,
            number(params, "aft_shoulder_length", number(params, "shoulder_length", 0.0)));
        const double shoulderOuter = std::max(0.0,
            number(params, "aft_shoulder_radius", number(params, "shoulder_outer_diameter", 0.0) * 0.5));
        const double shoulderThickness = std::clamp(
            number(params, "aft_shoulder_thickness", number(params, "shoulder_thickness", 0.0)), 0.0, shoulderOuter);
        const double shoulderInner = shoulderOuter - shoulderThickness;
        auto addAxialRing = [&](double ringOuter, double ringInner,
                                 double ringLength, double ringCg) {
            if (ringLength <= 0.0 || ringOuter <= 0.0) return;
            const double ringVolume = kPi * (ringOuter * ringOuter
                - ringInner * ringInner) * ringLength;
            const double ringMass = density * ringVolume;
            if (ringMass <= 0.0) return;
            const double oldMass = result.mass;
            const double oldCg = result.localCg;
            const double combinedMass = oldMass + ringMass;
            const double combinedCg = (oldMass * oldCg + ringMass * ringCg)
                / combinedMass;
            result.iAxial += 0.5 * ringMass * (ringOuter * ringOuter + ringInner * ringInner);
            result.iTransverse += ringMass * (
                3.0 * (ringOuter * ringOuter + ringInner * ringInner)
                + ringLength * ringLength) / 12.0;
            result.iTransverse += oldMass * (oldCg - combinedCg) * (oldCg - combinedCg)
                + ringMass * (ringCg - combinedCg) * (ringCg - combinedCg);
            result.volume += ringVolume;
            result.mass = combinedMass;
            result.localCg = combinedCg;
        };
        addAxialRing(shoulderOuter, shoulderInner, shoulderLength,
            result.length + shoulderLength / 2.0);
        if (params.value("aft_shoulder_capped", false)) {
            addAxialRing(shoulderInner, 0.0, shoulderThickness,
                result.length + shoulderLength - shoulderThickness / 2.0);
        }
        // SymmetricComponentCalc normalizes its nose area to the rocket
        // reference area.  A pod/booster nose therefore is not universally
        // CNa=2; it is 2*A_base/S_ref.
        result.cna = referenceDiameter > 1e-12
            ? 2.0 * radius * radius / ((referenceDiameter / 2.0) * (referenceDiameter / 2.0))
            : 0.0;
        double factor = 0.5;
        if (subtype == "Conical") factor = 2.0 / 3.0;
        if (subtype == "Tangent Ogive") {
            result.cp = position + tangentOgiveCpFromVolume(result.length, radius);
        } else {
            result.cp = position + factor * result.length;
        }
        result.aerodynamic = true;
    } else if (result.type == "Body Tube") {
        result.length = number(params, "length", 1.0);
        const double ro = number(params, "diameter", referenceDiameter) / 2.0;
        const double ri = std::max(ro - number(params, "thickness", 0.005), 0.0);
        result.volume = kPi * (ro * ro - ri * ri) * result.length;
        result.mass = density * result.volume;
        result.localCg = result.length / 2.0;
        result.iAxial = 0.5 * result.mass * (ro * ro + ri * ri);
        result.iTransverse = result.mass
            * (3.0 * (ro * ro + ri * ri) + result.length * result.length)
            / 12.0;
    } else if (result.type == "Transition") {
        result.length = number(params, "length", 0.1);
        const double rf =
            number(params, "fore_diameter", referenceDiameter) / 2.0;
        const double ra =
            number(params, "aft_diameter", referenceDiameter * 0.8) / 2.0;
        integrateShell(
            result.length,
            number(params, "thickness", 0.002),
            density,
            [&](double z) {
                return rf + (ra - rf) * z / std::max(result.length, 1e-9);
            },
            result
        );
        const double refRadius = referenceDiameter / 2.0;
        result.cna = Barrowman::transitionCna(rf, ra, refRadius);
        result.cp = position
            + Barrowman::transitionCpLocal(rf, ra, result.length);
        result.aerodynamic = std::abs(result.cna) > 1e-12;
    } else if (result.type == "Fins" || result.type == "Freeform Fin Set") {
        const bool isFreeform = result.type == "Freeform Fin Set";
        const double n = number(params, "n_fins", number(params, "n", 3.0));
        const double root = number(params, "root_chord", 0.25);
        const double tip = number(params, "tip_chord", 0.1);
        const double parameterSpan = number(params, "span", 0.12);
        const double sweep = number(params, "sweep_length", 0.1);
        const double thickness = number(params, "thickness", 0.003);
        const double bodyRadius =
            number(params, "body_radius", referenceDiameter / 2.0);
        FinPlanform::Metrics freeform;
        if (isFreeform) {
            freeform = freeform_planform::metricsFromParams(params);
        }
        const double span = isFreeform ? freeform.span : parameterSpan;
        const double area = isFreeform
            ? freeform.area : 0.5 * (root + tip) * span;
        // Some OpenRocket planforms share the legacy Fins transport shape
        // but have a non-trapezoidal physical area.  Preserve the aero
        // planform above while using the source's exact static area.
        const double massArea = number(params, "mass_planform_area_m2", area);
        result.length = isFreeform
            ? freeform.axialLength : std::max(root, sweep + tip);

        const std::string section = text(params, "cross_section", "square");
        double sectionFactor = 1.0;
        // OpenRocket FinSet relative-volume factors.  Rounded is nearly a
        // square planform (not a strongly thinned airfoil).
        if (section == "rounded") sectionFactor = 0.99;
        else if (section == "airfoil") sectionFactor = 0.65;
        else if (section == "diamond") sectionFactor = 0.5;
        else if (section == "biconvex") sectionFactor = 0.6;
        const double tabHeight = std::max(0.0, number(params, "tab_height", 0.0));
        const double tabLength = std::max(0.0, number(params, "tab_length", 0.0));
        // OpenRocket FinSet physical volume includes the root tab; it does
        // not alter the exposed planform used by CNa/CP.
        const double tabArea = tabHeight * tabLength;
        result.volume = n * (massArea + tabArea) * thickness * sectionFactor;
        result.mass = density * result.volume;
        if (isFreeform) {
            result.localCg = freeform.centroidX;
        } else {
            const double denominator = 3.0 * (root + tip);
            result.localCg = denominator > 1e-12
                ? (root * root + root * tip + tip * tip
                    + sweep * (root + 2.0 * tip)) / denominator
                : 0.0;
        }
        result.localCg = number(params, "mass_local_cg_m", result.localCg);
        const double radial = bodyRadius
            + (isFreeform ? freeform.centroidY : span / 2.0);
        result.iAxial = result.mass * radial * radial;
        result.iTransverse = result.mass
            * (result.length * result.length + radial * radial) / 12.0;

        const double midChord = root + tip;
        if (referenceDiameter > 0.0 && area > 1e-12
            && span > 1e-12 && (!isFreeform || freeform.valid)) {
            result.aerodynamic = true;
            result.fin = true;
            result.finArea = area;
            result.finSpan = span;
            result.finBodyRadius = bodyRadius;
            result.finCount = static_cast<int>(std::lround(n));
            result.finInterferenceCount = std::max(
                1, static_cast<int>(std::lround(number(
                    params, "interference_fin_count", n))));
            if (isFreeform) {
                result.finCosGamma = freeform.midChordCosine;
                result.finAspectRatio = 2.0 * span * span / area;
                result.finMacLength = freeform.macLength;
                result.finMacLead = freeform.macLeadingEdge;
                result.cna = finCnaAtMach(
                    result, 0.0, kPi * referenceDiameter
                        * referenceDiameter / 4.0);
                result.cp = position + freeform.quarterChordCp;
            } else if (midChord > 1e-12) {
                const double lf = std::sqrt(sweep * sweep + span * span);
                const double raw = (4.0 * n
                    * std::pow(span / referenceDiameter, 2.0))
                    / (1.0 + std::sqrt(1.0
                        + std::pow(2.0 * lf / midChord, 2.0)));
                result.cna = raw
                    * (1.0 + bodyRadius / (bodyRadius + span));
                result.cp = position
                    + (sweep / 3.0) * (root + 2.0 * tip) / midChord
                    + (root * root + tip * tip + root * tip)
                        / (6.0 * midChord);
                result.finCosGamma = span / std::max(std::hypot(
                    span, sweep + 0.5 * (tip - root)), 1e-12);
                result.finAspectRatio = 2.0 * span * span / area;
                result.finMacLength = 2.0
                    * (root*root + root*tip + tip*tip)
                    / (3.0 * midChord);
                result.finMacLead = sweep * (root + 2.0*tip)
                    / (3.0 * midChord);
            }
        }
    } else if (result.type == "Tube Fin Set") {
        // OpenRocket TubeFinSet is a circular array of individual annular
        // cylinders.  The tube centres lie at bodyRadius + outerRadius;
        // aggregate them with the parallel-axis theorem, never as one large
        // annulus around the airframe.
        result.length = number(params, "length", 0.0762);
        const int count = std::max(1, static_cast<int>(std::lround(
            number(params, "fin_count", 6.0))));
        const double ro = std::max(0.0, number(params, "outer_radius", 0.0127));
        const double ri = std::clamp(number(params, "inner_radius",
            ro - number(params, "thickness", 0.00033)), 0.0, ro);
        const double bodyRadius = std::max(0.0, number(params, "body_radius",
            referenceDiameter / 2.0));
        const double oneVolume = kPi * (ro * ro - ri * ri) * result.length;
        const double oneMass = density * oneVolume;
        const double oneAxial = 0.5 * oneMass * (ro * ro + ri * ri);
        const double oneTransverse = oneMass * (
            3.0 * (ro * ro + ri * ri) + result.length * result.length) / 12.0;
        const double offset = bodyRadius + ro;
        result.volume = oneVolume * count;
        result.mass = oneMass * count;
        result.localCg = result.length / 2.0;
        result.iAxial = count * (oneAxial + oneMass * offset * offset);
        result.iTransverse = count * (oneTransverse + oneMass * offset * offset / 2.0);
        // OpenRocket TubeFinSetCalc: low-Mach CP is quarter chord and CNa
        // follows Ribner's ring-airfoil relation, normalized by ref area.
        if (referenceDiameter > 1e-12 && ri > 1e-12 && result.length > 1e-12) {
            const double ar = 2.0 * ri / result.length;
            const double arPrime = 2.0 * ar / kPi;
            const double cnaConst = 2.0 * (arPrime / (1.0 + arPrime))
                * kPi * kPi * ri * result.length;
            const double refArea = kPi * referenceDiameter * referenceDiameter / 4.0;
            // TubeFinSetCalc is evaluated once for every physical tube by
            // OpenRocket's instance map.  ``cnaConst`` is deliberately the
            // Ribner relation for one tube, so aggregate the circular array
            // here rather than treating it as one annular fin.
            result.cna = static_cast<double>(count) * cnaConst / refArea;
            result.cp = position + 0.25 * result.length;
            result.aerodynamic = true;
            result.tubeFin = true;
            result.tubeFinAspectRatio = ar;
        }
    } else if (result.type == "Sleeve") {
        // Sleeve is an annular shell on top of the parent body.  Its source
        // inner radius was resolved by the importer when automatic.
        result.length = number(params, "length", 0.05);
        const double ri = std::max(0.0, number(params, "inner_radius",
            referenceDiameter / 2.0));
        const double ro = std::max(ri, number(params, "outer_radius",
            ri + number(params, "thickness", 0.001)));
        result.volume = kPi * (ro * ro - ri * ri) * result.length;
        result.mass = density * result.volume;
        result.localCg = result.length / 2.0;
        result.iAxial = 0.5 * result.mass * (ri * ri + ro * ro);
        result.iTransverse = result.mass * (
            3.0 * (ri * ri + ro * ro) + result.length * result.length) / 12.0;
    } else if (result.type == "Pod Set" || result.type == "Parallel Stage") {
        // This is intentionally a definition-level assembly.  Its children
        // stay in the serialized tree and carry their own geometry; the
        // assembly itself has no invented shell/mass.
        result.length = 0.0;
    } else if (result.type == "Motor") {
        result.length = number(params, "length", 0.2);
        const double radius = number(params, "radius", referenceDiameter / 2.0);
        result.motorDryMass = number(params, "dry_mass_kg", 0.0);
        result.motorPropellantMass = number(params, "propellant_mass_kg", 0.0);
        // A database-backed motor may supply an actual t=0 CG curve.  The
        // midpoint remains only the explicit fallback for formats without it.
        result.localCg = number(params, "motor_local_cg_m", result.length / 2.0);
        result.motorDesignation = text(params, "designation");
        result.motorManufacturer = text(params, "manufacturer");
        result.motorCaseInfo = text(params, "case_info");
        result.motorPropellantInfo = text(params, "propellant_info");
        result.motorDelays = text(params, "delays");
        result.motorSparky = params.value("sparky", false);
        result.motorAvailable = params.value("available", true);
        result.motorCluster.layout = motorClusterLayoutFromString(
            text(params, "cluster_layout", "single"));
        result.motorCluster.count = std::max(
            1, static_cast<int>(std::lround(number(params, "cluster_count", 1.0))));
        result.motorCluster.radius = number(params, "cluster_radius_m", 0.0);
        result.motorCluster.rotationRad = number(
            params, "cluster_rotation_deg", 0.0) * kPi / 180.0;
        if (result.motorCluster.count == 1) {
            result.motorCluster.layout = MotorClusterLayout::Single;
            result.motorCluster.radius = 0.0;
        }
        result.motorClusterOffsets = ::motorClusterPositions(result.motorCluster);
        const double singleMass = result.motorDryMass
            + result.motorPropellantMass;
        result.mass = singleMass * result.motorCluster.count;
        result.volume = kPi * radius * radius * result.length
            * result.motorCluster.count;
        const double singleAxial = 0.5 * singleMass * radius * radius;
        const double singleTransverse = singleMass
            * (3.0 * radius * radius + result.length * result.length) / 12.0;
        result.inertiaLocal(0, 0) = singleTransverse
            * result.motorCluster.count;
        result.inertiaLocal(1, 1) = singleTransverse
            * result.motorCluster.count;
        result.inertiaLocal(2, 2) = singleAxial
            * result.motorCluster.count;
        for (const auto& offset : result.motorClusterOffsets) {
            const double x = offset.x();
            const double y = offset.y();
            result.inertiaLocal(0, 0) += singleMass * y * y;
            result.inertiaLocal(1, 1) += singleMass * x * x;
            result.inertiaLocal(0, 1) -= singleMass * x * y;
            result.inertiaLocal(1, 0) -= singleMass * x * y;
            result.inertiaLocal(2, 2) += singleMass * (x * x + y * y);
        }
        result.iTransverse = 0.5 * (
            result.inertiaLocal(0, 0) + result.inertiaLocal(1, 1));
        result.iAxial = result.inertiaLocal(2, 2);
        result.motor = true;
    } else if (result.type == "Internal Mass") {
        result.length = number(params, "length", 0.2);
        const double radius = number(params, "radius", referenceDiameter / 2.0);
        result.mass = number(params, "mass", 0.642);
        result.volume = kPi * radius * radius * result.length;
        // MassComponent's packed length is its static physical extent.  Its
        // centroid is therefore at the packed-length midpoint, not its front.
        result.localCg = result.length / 2.0;
        result.iAxial = 0.5 * result.mass * radius * radius;
        result.iTransverse = result.mass
            * (3.0 * radius * radius + result.length * result.length) / 12.0;
    } else if (result.type == "Rail Button") {
        result.length = number(params, "height", 0.0155);
        const double outerDiameter = number(params, "outer_diameter", 0.016);
        const double innerDiameter = number(params, "inner_diameter", 0.012);
        const double ro = outerDiameter / 2.0;
        const double ri = innerDiameter / 2.0;
        const double baseHeight = std::max(0.0, number(params, "base_height", 0.0));
        const double flangeHeight = std::max(0.0, number(params, "flange_height", 0.0));
        const double screwHeight = std::max(0.0, number(params, "screw_height", 0.0));
        const double innerHeight = std::max(0.0, result.length - baseHeight - flangeHeight);
        const int instances = std::max(1, static_cast<int>(std::lround(
            number(params, "instance_count", 1.0))));
        const double instanceSeparation = number(
            params, "instance_separation_m", 0.0);
        // RailButton's physical model consists of a full-diameter base and
        // flange, an inner-diameter standoff, and an optional screw cap.
        const double oneVolume = kPi * (ro * ro * (baseHeight + flangeHeight)
            + ri * ri * innerHeight + (2.0 / 3.0) * ro * ro * screwHeight);
        result.volume = oneVolume * instances;
        result.mass = density * result.volume;
        result.axialInstanceOffsets.clear();
        for (int index = 0; index < instances; ++index) {
            result.axialInstanceOffsets.push_back(index * instanceSeparation);
        }
        // The physical button centroid is intentionally not changed here.
        // Aggregate CG is calculated later from the canonical instance
        // offsets, so placement and geometry-centroid semantics stay distinct.
        result.localCg = result.length / 2.0;
        result.iAxial = instances * 0.5 * result.mass / instances * (ro * ro + ri * ri);
        result.iTransverse = result.mass * (
            3.0 * (ro * ro + ri * ri) + result.length * result.length) / 12.0;
    } else if (result.type == "Launch Lug"
        || result.type == "Inner Tube" || result.type == "Motor Mount") {
        result.length = number(params, "length",
            result.type == "Rail Button" ? number(params, "height", 0.0155)
            : (result.type == "Launch Lug" ? 0.05 : 0.3));
        const double ri = number(params, "inner_diameter",
            (result.type == "Launch Lug" || result.type == "Rail Button") ? 0.012 : 0.038) / 2.0;
        const double ro = number(params, "outer_diameter",
            (result.type == "Launch Lug" || result.type == "Rail Button") ? 0.016 : 0.042) / 2.0;
        result.volume = kPi * std::max(ro * ro - ri * ri, 0.0) * result.length;
        result.mass = density * result.volume;
        result.localCg = result.length / 2.0;
        result.iAxial = 0.5 * result.mass * (ri * ri + ro * ro);
        result.iTransverse = result.mass
            * (3.0 * (ri * ri + ro * ro) + result.length * result.length)
            / 12.0;
    } else if (result.type == "Centering Ring") {
        result.length = number(params, "thickness", 0.003);
        const double ri = number(params, "inner_diameter",
            referenceDiameter * 0.5) / 2.0;
        const double ro = number(params, "outer_diameter",
            referenceDiameter) / 2.0;
        result.volume = kPi * std::max(ro * ro - ri * ri, 0.0) * result.length;
        result.mass = density * result.volume;
        result.localCg = result.length / 2.0;
        result.iAxial = 0.5 * result.mass * (ri * ri + ro * ro);
        result.iTransverse = result.mass
            * (3.0 * (ri * ri + ro * ro) + result.length * result.length)
            / 12.0;
    } else if (result.type == "Bulkhead") {
        result.length = number(params, "thickness", 0.005);
        const double radius =
            number(params, "diameter", referenceDiameter) / 2.0;
        result.volume = kPi * radius * radius * result.length;
        result.mass = density * result.volume;
        result.localCg = result.length / 2.0;
        result.iAxial = 0.5 * result.mass * radius * radius;
        result.iTransverse = result.mass
            * (3.0 * radius * radius + result.length * result.length) / 12.0;
    } else if (result.type == "Engine Block") {
        result.length = number(params, "length", 0.005);
        const double ro = number(params, "outer_diameter", referenceDiameter) / 2.0;
        const double thickness = std::max(0.0, number(params, "thickness", 0.005));
        const double ri = std::max(0.0, ro - thickness);
        result.volume = kPi * std::max(ro * ro - ri * ri, 0.0) * result.length;
        result.mass = density * result.volume;
        result.localCg = result.length / 2.0;
        result.iAxial = 0.5 * result.mass * (ri * ri + ro * ro);
        result.iTransverse = result.mass
            * (3.0 * (ri * ri + ro * ro) + result.length * result.length) / 12.0;
    } else if (result.type == "Parachute") {
        result.mass = number(params, "packed_mass", 0.15);
        result.length = number(params, "packed_length", 0.0);
        result.localCg = result.length / 2.0;
    } else if (result.type == "Shock Cord") {
        const double diameter = number(params, "cord_diameter", 0.012);
        const double cordLength = number(params, "cord_length", 3.0);
        result.volume = kPi * std::pow(diameter / 2.0, 2.0) * cordLength;
        // OpenRocket line materials carry kg/m; preserve that source value
        // instead of resolving an unrelated bulk/line catalog default.
        const double sourceLineDensity = number(params,
            "source_line_density_kg_m", -1.0);
        result.mass = (sourceLineDensity >= 0.0 ? sourceLineDensity : density)
            * cordLength;
        result.length = number(params, "packed_length", 0.0);
        result.localCg = result.length / 2.0;
    }

    if (!result.motor) {
        result.inertiaLocal(0, 0) = result.iTransverse;
        result.inertiaLocal(1, 1) = result.iTransverse;
        result.inertiaLocal(2, 2) = result.iAxial;
    }

    // A component with multiple physical instances spread along the axis
    // (Rail Button, or any other component using instance_count/
    // instance_separation_m) has its mass already correctly totaled above
    // (iTransverse/iAxial are already "N x one instance's own shape
    // inertia"), but that alone is only the Sigma(I_instance,cg) term of the
    // parallel-axis sum -- the Sigma(m_i * d_i^2) term for each instance's
    // OWN offset from the group's shared centroid is still missing. For a
    // purely axial (z-only) instance spread the parallel-axis tensor
    // contribution is m*(||d||^2*I - d*d^T) with d = (0,0,dz), which is
    // nonzero only on the transverse (x,x)/(y,y) diagonal -- the roll/axial
    // term (z,z) is unaffected by an axial-only spread. Generic: not
    // special-cased to any one component type.
    if (!result.motor && result.axialInstanceOffsets.size() > 1) {
        const std::size_t instanceCount = result.axialInstanceOffsets.size();
        const double perInstanceMass = result.mass / static_cast<double>(instanceCount);
        double meanOffset = 0.0;
        for (const double offset : result.axialInstanceOffsets) meanOffset += offset;
        meanOffset /= static_cast<double>(instanceCount);
        double spread = 0.0;
        for (const double offset : result.axialInstanceOffsets) {
            const double d = offset - meanOffset;
            spread += perInstanceMass * d * d;
        }
        result.inertiaLocal(0, 0) += spread;
        result.inertiaLocal(1, 1) += spread;
        result.iTransverse += spread;
    }

    // InnerTube and similar OpenRocket Clusterable components are one design
    // definition with several real physical mounts.  Materialize their mass
    // and inertia around the cluster centre without flattening the imported
    // component tree or double-counting their children.
    const int physicalInstances = std::max(1, static_cast<int>(std::lround(
        number(params, "physical_instance_count", 1.0))));
    if (!result.motor && physicalInstances > 1) {
        MotorClusterConfiguration cluster;
        cluster.layout = motorClusterLayoutFromString(text(
            params, "physical_cluster_layout", "ring"));
        cluster.count = physicalInstances;
        cluster.radius = std::max(0.0, number(
            params, "physical_cluster_radius_m", 0.0));
        cluster.rotationRad = number(params, "physical_cluster_rotation_deg", 0.0)
            * kPi / 180.0;
        const std::vector<Eigen::Vector2d> offsets = motorClusterPositions(cluster);
        const double singleMass = result.mass;
        const double singleVolume = result.volume;
        const Eigen::Matrix3d singleInertia = result.inertiaLocal;
        result.mass = singleMass * physicalInstances;
        result.volume = singleVolume * physicalInstances;
        result.inertiaLocal = Eigen::Matrix3d::Zero();
        for (const auto& offset : offsets) {
            const Eigen::Vector3d displacement(offset.x(), offset.y(), 0.0);
            result.inertiaLocal += singleInertia + singleMass * (
                displacement.squaredNorm() * Eigen::Matrix3d::Identity()
                - displacement * displacement.transpose());
        }
        result.iTransverse = 0.5 * (result.inertiaLocal(0, 0)
            + result.inertiaLocal(1, 1));
        result.iAxial = result.inertiaLocal(2, 2);
    }

    if (enabled(overrides, "mass")) {
        result.mass = number(overrides["mass"], "value", result.mass);
    }
    if (enabled(overrides, "cg")) {
        const double value = number(overrides["cg"], "value", result.localCg);
        result.localCg = text(overrides["cg"], "type", "Local") == "Absolute"
            ? value - position : value;
    }
    if (enabled(overrides, "cna")) {
        result.cna = number(overrides["cna"], "value", result.cna);
        result.aerodynamic = true;
    }
    if (enabled(overrides, "cp")) {
        result.cp = number(overrides["cp"], "value", result.cp);
        result.aerodynamic = true;
    }
    return result;
}

// Convert the UI's parent_id graph into absolute component positions before
// mass/aero/drag analysis.  Stage Separator is a virtual AxialStage container;
// it has no standalone mass result, but owns an ordered child group.
inline json resolveComponentTreeLayout(const json& input, double referenceDiameter) {
    json resolved = input;
    if (!resolved.contains("components") || !resolved["components"].is_array()) {
        return resolved;
    }

    struct Node {
        std::size_t sourceIndex = 0;
        std::string id;
        std::string parentId;
        std::string type;
        std::string mode;
        double offset = 0.0;
        double length = 0.0;
        double position = 0.0;
    };

    std::unordered_set<std::string> stageIds;
    std::unordered_map<std::string, std::size_t> nodeById;
    std::vector<Node> nodes;
    for (std::size_t i = 0; i < resolved["components"].size(); ++i) {
        const json& component = resolved["components"][i];
        const std::string type = text(component, "type");
        const std::string id = text(component, "id", "component-" + std::to_string(i));
        if (type == "Stage Separator") {
            stageIds.insert(id);
            continue;
        }
        const ComponentResult preview = analyzeComponent(component, 0.0, referenceDiameter);
        nodeById[id] = nodes.size();
        double placementLength = preview.length;
        if (type == "Fins" || type == "Freeform Fin Set") {
            const json& params = component.contains("params")
                ? component["params"] : component;
            // BOTTOM placement uses the root chord attached to the parent,
            // not a swept tip that may extend farther aft.
            placementLength = number(params, "root_chord", preview.length);
        }
        nodes.push_back({
            i, id, text(component, "parent_id"), type,
            text(component, "pos_mode", "Automatic (Stacked)"),
            number(component, "pos_value", 0.0), placementLength, 0.0,
        });
    }

    std::unordered_map<std::string, std::vector<std::size_t>> children;
    const std::string rocketRoot = "__rocket__";
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const std::string& parentId = nodes[index].parentId;
        const bool knownParent = nodeById.count(parentId) || stageIds.count(parentId);
        children[knownParent ? parentId : rocketRoot].push_back(index);
    }

    // ComponentAssembly.updateBounds() derives its axial length from the
    // sequential child chain.  A PodSet/ParallelStage positioned from its
    // parent's bottom must therefore use that derived length, not the zero
    // definition length returned by analyzeComponent().
    std::function<double(const std::string&)> assemblyLength;
    assemblyLength = [&](const std::string& parentId) {
        const auto found = children.find(parentId);
        if (found == children.end()) return 0.0;
        double sequentialTail = 0.0;
        double furthestTail = 0.0;
        for (const std::size_t index : found->second) {
            Node& child = nodes[index];
            if (child.type == "Pod Set" || child.type == "Parallel Stage") {
                child.length = assemblyLength(child.id);
            }
            if (child.mode == "Reverse from Tail") continue;
            const double local = child.mode == "Automatic (Stacked)"
                ? sequentialTail
                : (child.mode == "Relative Offset" ? sequentialTail + child.offset : child.offset);
            const double end = local + child.length;
            furthestTail = std::max(furthestTail, end);
            if (child.mode == "Automatic (Stacked)" || child.mode == "Relative Offset") {
                sequentialTail = end;
            }
        }
        return furthestTail;
    };
    for (Node& node : nodes) {
        if (node.type == "Pod Set" || node.type == "Parallel Stage") {
            node.length = assemblyLength(node.id);
        }
    }

    std::unordered_set<std::string> visiting;
    std::function<double(const std::string&, double, double)> layoutGroup;
    layoutGroup = [&](const std::string& parentId, double origin, double parentLength) {
        const auto found = children.find(parentId);
        if (found == children.end()) return 0.0;
        if (!visiting.insert(parentId).second) {
            throw std::runtime_error("Component parent_id cycle at " + parentId + ".");
        }
        double sequentialTail = 0.0;
        double furthestTail = 0.0;
        std::vector<std::size_t> fromBottom;
        for (const std::size_t index : found->second) {
            Node& node = nodes[index];
            if (node.mode == "Reverse from Tail") {
                fromBottom.push_back(index);
                continue;
            }
            double local = node.offset;
            if (node.mode == "Automatic (Stacked)") local = sequentialTail;
            else if (node.mode == "Relative Offset") local = sequentialTail + node.offset;
            node.position = origin + local;
            const double end = local + node.length;
            furthestTail = std::max(furthestTail, end);
            if (node.mode == "Automatic (Stacked)" || node.mode == "Relative Offset") {
                sequentialTail = end;
            }
            layoutGroup(node.id, node.position, node.length);
        }
        const double tailReference = parentLength > 0.0 ? parentLength : furthestTail;
        for (const std::size_t index : fromBottom) {
            Node& node = nodes[index];
            const double local = tailReference - node.length - node.offset;
            node.position = origin + local;
            furthestTail = std::max(furthestTail, local + node.length);
            layoutGroup(node.id, node.position, node.length);
        }
        visiting.erase(parentId);
        return furthestTail;
    };

    // Legacy flat designs are still a single sequential root group.
    layoutGroup(rocketRoot, 0.0, 0.0);
    double stageOrigin = 0.0;
    for (const auto& component : resolved["components"]) {
        if (text(component, "type") != "Stage Separator") continue;
        stageOrigin += layoutGroup(text(component, "id"), stageOrigin, 0.0);
    }

    for (const Node& node : nodes) {
        json& component = resolved["components"][node.sourceIndex];
        component["pos_mode"] = "Absolute";
        component["pos_value"] = node.position;
        component["resolved_reverse_from_tail"] = node.mode == "Reverse from Tail";
        component["resolved_parent_id"] = node.parentId;
    }
    return resolved;
}

// OpenRocket keeps PodSet and ParallelStage as definition-level assemblies.
// Their children remain in the document tree once, while FlightConfiguration
// resolves a physical child subtree for every ring instance.  Preserve that
// canonical input hierarchy, but expand it into an analysis-only physical list
// before mass, inertia and aerodynamic aggregation.
inline json resolvePhysicalInstances(const json& resolvedInput,
                                     double referenceDiameter) {
    json physical = resolvedInput;
    if (!resolvedInput.contains("components") || !resolvedInput["components"].is_array()) {
        return physical;
    }

    const auto& definitions = resolvedInput["components"];
    std::unordered_map<std::string, std::size_t> indexById;
    std::unordered_map<std::string, std::vector<std::size_t>> children;
    for (std::size_t i = 0; i < definitions.size(); ++i) {
        const std::string id = text(definitions[i], "id", "component-" + std::to_string(i));
        indexById[id] = i;
    }
    for (std::size_t i = 0; i < definitions.size(); ++i) {
        children[text(definitions[i], "parent_id")].push_back(i);
    }

    const auto isAssembly = [&](const json& component) {
        const std::string type = text(component, "type");
        return type == "Pod Set" || type == "Parallel Stage";
    };
    const auto componentRadius = [&](const json& component) {
        const json params = component.contains("params") ? component["params"] : json::object();
        const std::string type = text(component, "type");
        if (type == "Nose Cone" || type == "Body Tube") {
            return number(params, "diameter", number(params, "outer_diameter", 0.0)) / 2.0;
        }
        if (type == "Transition") {
            return std::max(number(params, "fore_diameter"), number(params, "aft_diameter")) / 2.0;
        }
        if (type == "Tube Fin Set") return number(params, "body_radius", 0.0)
            + number(params, "outer_radius", 0.0);
        if (type == "Fins" || type == "Freeform Fin Set") {
            return number(params, "body_radius", 0.0) + number(params, "span", 0.0);
        }
        return 0.0;
    };
    const auto assemblyBoundingRadius = [&](std::size_t index) {
        // ComponentAssembly.getBoundingRadius() intentionally considers only
        // direct BodyTube/Transition children, not their attached fin span.
        // This is the RadiusMethod.RELATIVE contract used by PodSet and
        // ParallelStage instance offsets.
        double radius = 0.0;
        const auto found = children.find(text(definitions[index], "id"));
        if (found != children.end()) {
            for (const std::size_t child : found->second) {
                const std::string type = text(definitions[child], "type");
                if (type == "Body Tube" || type == "Transition") {
                    radius = std::max(radius, componentRadius(definitions[child]));
                }
            }
        }
        return radius;
    };

    struct Transform {
        double x = 0.0;
        double y = 0.0;
        double angle = 0.0;
    };
    json assemblies = json::array();
    json components = json::array();
    std::function<void(std::size_t, Transform, const std::string&, int,
                       const std::string&)> visit;
    visit = [&](std::size_t index, Transform parentTransform,
                const std::string& assemblyId, int instanceIndex,
                const std::string& physicalParentId) {
        const json& definition = definitions[index];
        const std::string id = text(definition, "id", "component-" + std::to_string(index));
        const auto childNodes = children.find(id);
        if (text(definition, "type") == "Stage Separator") {
            if (childNodes != children.end()) {
                for (const std::size_t child : childNodes->second) {
                    visit(child, parentTransform, assemblyId, instanceIndex,
                          physicalParentId);
                }
            }
            return;
        }

        if (isAssembly(definition)) {
            const json params = definition.contains("params") ? definition["params"] : json::object();
            const int count = std::max(1, static_cast<int>(std::lround(
                number(params, "instance_count", 1.0))));
            const std::string parentId = text(definition, "parent_id");
            double parentRadius = referenceDiameter / 2.0;
            if (const auto parent = indexById.find(parentId); parent != indexById.end()) {
                parentRadius = componentRadius(definitions[parent->second]);
            }
            const double childRadius = assemblyBoundingRadius(index);
            const double configuredRadius = number(params, "radius_offset_m", 0.0);
            const bool relativeRadius = text(params, "radius_method", "relative") != "absolute";
            const double radial = std::max(0.0, configuredRadius
                + (relativeRadius ? parentRadius + childRadius : 0.0));
            const double baseAngle = number(params, "angle_offset_deg", 0.0) * kPi / 180.0;
            json assembly = {
                {"id", id}, {"type", text(definition, "type")},
                {"instance_count", count}, {"instances", json::array()},
            };
            for (int copy = 0; copy < count; ++copy) {
                const double angle = baseAngle + 2.0 * kPi * copy / count;
                const Transform instanceTransform{
                    parentTransform.x + radial * std::cos(parentTransform.angle + angle),
                    parentTransform.y + radial * std::sin(parentTransform.angle + angle),
                    parentTransform.angle + angle,
                };
                assembly["instances"].push_back({
                    {"index", copy},
                    {"radial_position_m", {instanceTransform.x, instanceTransform.y}},
                    {"angle_deg", instanceTransform.angle * 180.0 / kPi},
                });
                if (childNodes != children.end()) {
                    for (const std::size_t child : childNodes->second) {
                        visit(child, instanceTransform, id, copy,
                              id + "::physical::" + std::to_string(copy));
                    }
                }
            }
            assemblies.push_back(std::move(assembly));
            return;
        }

        json instance = definition;
        // AxialStage/Stage Separator is a virtual container.  Its symmetric
        // airframe chain continues into the next active stage in OpenRocket;
        // do not split that chain merely because the importer preserved the
        // virtual parent id.
        const std::string definitionParentId = text(definition, "parent_id");
        if (assemblyId.empty() && physicalParentId.empty()) {
            const auto parent = indexById.find(definitionParentId);
            if (parent != indexById.end()
                && text(definitions[parent->second], "type") == "Stage Separator") {
                instance["parent_id"] = "";
            }
        }
        if (!assemblyId.empty()) {
            const double localRadius = std::max(number(instance, "radial_offset_m", 0.0), 0.0);
            const double localAngle = number(instance, "angle_deg", 0.0) * kPi / 180.0;
            const double orientation = parentTransform.angle + localAngle;
            const double x = parentTransform.x + localRadius * std::cos(orientation);
            const double y = parentTransform.y + localRadius * std::sin(orientation);
            instance["id"] = id + "::" + assemblyId + "::" + std::to_string(instanceIndex);
            instance["source_parent_id"] = definitionParentId;
            instance["parent_id"] = physicalParentId;
            instance["resolved_parent_id"] = assemblyId;
            instance["source_assembly_id"] = assemblyId;
            instance["physical_instance_index"] = instanceIndex;
            instance["physical_transform"] = {
                {"radial_position_m", {x, y}},
                {"angle_deg", orientation * 180.0 / kPi},
            };
            instance["radial_mode"] = "Free Offset";
            instance["radial_offset_m"] = std::hypot(x, y);
            instance["angle_deg"] = std::atan2(y, x) * 180.0 / kPi;
            instance["physical_orientation_deg"] = orientation * 180.0 / kPi;
        }
        // Fin/body interference needs the actual local parent radius.  The
        // top-level reference diameter is only correct for centerline fins;
        // booster and pod fin sets inherit their own BodyTube radius.
        const std::string type = text(instance, "type");
        if (type == "Fins" || type == "Freeform Fin Set") {
            const auto parent = indexById.find(text(definition, "parent_id"));
            if (parent != indexById.end()) {
                instance["params"]["body_radius"] = componentRadius(definitions[parent->second]);
            }
        }
        components.push_back(std::move(instance));
        if (childNodes != children.end()) {
            for (const std::size_t child : childNodes->second) {
                visit(child, parentTransform, assemblyId, instanceIndex,
                      text(instance, "id"));
            }
        }
    };

    for (std::size_t index = 0; index < definitions.size(); ++index) {
        const std::string parentId = text(definitions[index], "parent_id");
        if (parentId.empty() || !indexById.count(parentId)) {
            visit(index, {}, "", -1, "");
        }
    }
    physical["components"] = std::move(components);
    physical["physical_assemblies"] = std::move(assemblies);
    return physical;
}

// FinSetCalc does not use only the fin count of the current set for
// interference.  It counts all overlapping FinSet siblings on the same
// physical parent (with the short-root-chord exception).  Run this after
// PodSet/ParallelStage resolution so independent physical copies never see
// one another as overlapping fins.
inline json resolveFinInterference(json physical) {
    if (!physical.contains("components") || !physical["components"].is_array()) {
        return physical;
    }
    auto isFin = [](const json& component) {
        const std::string type = text(component, "type");
        return type == "Fins" || type == "Freeform Fin Set";
    };
    auto finLength = [](const json& component) {
        const json params = component.value("params", json::object());
        if (text(component, "type") == "Freeform Fin Set") {
            return freeform_planform::metricsFromParams(params).axialLength;
        }
        return std::max(number(params, "root_chord"),
                        number(params, "sweep_length") + number(params, "tip_chord"));
    };
    std::unordered_map<std::string, std::vector<std::size_t>> finsByParent;
    for (std::size_t i = 0; i < physical["components"].size(); ++i) {
        if (isFin(physical["components"][i])
            && isExternallyExposedForAero(physical["components"][i])) {
            finsByParent[text(physical["components"][i], "parent_id")].push_back(i);
        }
    }
    for (const auto& entry : finsByParent) {
        for (const std::size_t index : entry.second) {
            json& fin = physical["components"][index];
            const json params = fin.value("params", json::object());
            const int ownCount = std::max(1, static_cast<int>(std::lround(
                number(params, "n_fins", number(params, "n", 1.0)))));
            const double lead = number(fin, "pos_value");
            const double trail = lead + finLength(fin);
            int count = ownCount;
            if (trail - lead >= 0.007) {
                count = 0;
                for (const std::size_t siblingIndex : entry.second) {
                    const json& sibling = physical["components"][siblingIndex];
                    const double siblingLead = number(sibling, "pos_value");
                    const double siblingTrail = siblingLead + finLength(sibling);
                    if (siblingLead < trail - 0.005 && siblingTrail > lead + 0.005) {
                        const json siblingParams = sibling.value("params", json::object());
                        count += std::max(1, static_cast<int>(std::lround(
                            number(siblingParams, "n_fins", number(siblingParams, "n", 1.0)))));
                    }
                }
            }
            fin["params"]["interference_fin_count"] = std::max(count, ownCount);
        }
    }
    return physical;
}

inline double effectiveReferenceDiameter(const json& input) {
    double diameter = input.contains("reference_diameter_m")
        ? input["reference_diameter_m"].get<double>()
        : (input.contains("reference_diameter")
            ? input["reference_diameter"].get<double>() : 0.06);
    if (!input.contains("components") || !input["components"].is_array()) {
        return std::max(diameter, 1.0e-3);
    }
    for (const auto& component : input["components"]) {
        if (!isExternallyExposedForAero(component)) {
            continue;
        }
        const std::string type = text(component, "type");
        if (!component.contains("params") || !component["params"].is_object()) {
            continue;
        }
        const json& params = component["params"];
        if (type == "Nose Cone" || type == "Body Tube") {
            diameter = std::max(diameter, number(
                params, "diameter", number(params, "outer_diameter", 0.0)));
        } else if (type == "Transition") {
            diameter = std::max({diameter,
                number(params, "fore_diameter"),
                number(params, "aft_diameter")});
        }
    }
    return std::max(diameter, 1.0e-3);
}

// Restricts analysis to a caller-selected subset of components, identified
// by id. This lets a multi-stage flight phase ask the canonical calculator
// for the mass/CG/inertia/aero of just the currently-attached vehicle,
// without a second mass/aero implementation anywhere else. An absent or
// empty filter means "analyze the complete rocket exactly as today" --
// existing single-stage callers are unaffected.
inline json applyActiveComponentFilter(const json& input) {
    if (!input.contains("active_component_ids")
        || !input["active_component_ids"].is_array()
        || input["active_component_ids"].empty()
        || !input.contains("components") || !input["components"].is_array()) {
        return input;
    }
    std::unordered_set<std::string> activeIds;
    for (const auto& id : input["active_component_ids"]) {
        if (id.is_string()) activeIds.insert(id.get<std::string>());
    }
    json filtered = input;
    json keptComponents = json::array();
    for (const auto& component : input["components"]) {
        if (activeIds.count(text(component, "id"))) {
            keptComponents.push_back(component);
        }
    }
    filtered["components"] = std::move(keptComponents);
    return filtered;
}

inline json analyze(
    const json& rawInput,
    const RoketSiM::AeroExtensions::AeroExtensionPipeline& extensionPipeline
) {
    if (!rawInput.contains("components") || !rawInput["components"].is_array()) {
        throw std::runtime_error("Design input requires a components array.");
    }
    const json input = applyActiveComponentFilter(rawInput);
    const double referenceDiameter = effectiveReferenceDiameter(input);

    const json resolvedInput = resolveComponentTreeLayout(input, referenceDiameter);
    const json physicalInput = resolveFinInterference(
        resolvePhysicalInstances(resolvedInput, referenceDiameter));
    std::vector<ComponentResult> components;
    double currentZ = 0.0;
    double totalLength = 0.0;
    for (const auto& component : physicalInput["components"]) {
        if (text(component, "type") == "Stage Separator") continue;
        const std::string mode = text(
            component, "pos_mode", "Automatic (Stacked)");
        double position = number(component, "pos_value", 0.0);
        const bool reverseFromTail = component.value("resolved_reverse_from_tail", false)
            || mode == "Reverse from Tail";
        if (mode == "Automatic (Stacked)") position = currentZ;
        else if (mode == "Relative Offset") position = currentZ + position;
        ComponentResult result =
            analyzeComponent(component, position, referenceDiameter);
        // Axial "bottom" placement does not mirror mass geometry, but an
        // aft-facing NoseCone/Transition is aerodynamically reversed.  This
        // is how OpenRocket represents pod tail cones while retaining the
        // component's local mass frame.
        const bool isFlipped = component.value("params", json::object())
            .value("is_flipped", false);
        if ((reverseFromTail || isFlipped) && result.aerodynamic
            && (result.type == "Nose Cone" || result.type == "Transition")) {
            result.cp = position + result.length - (result.cp - position);
            result.cna = -result.cna;
        }
        // Exposure is independent from mass/CG/inertia.  The component stays
        // in the physical aggregation below while leaving aero aggregation.
        if (!isExternallyExposedForAero(component)) {
            result.aerodynamic = false;
        }
        components.push_back(result);
        totalLength = std::max(totalLength, position + result.length);
        if (mode == "Automatic (Stacked)" || mode == "Relative Offset") {
            currentZ = position + result.length;
        }
    }

    double totalMass = 0.0;
    Eigen::Vector3d firstMoment = Eigen::Vector3d::Zero();
    double cnaTotal = 0.0;
    double cpMoment = 0.0;
    for (const auto& component : components) {
        totalMass += component.mass;
        firstMoment += component.mass * Eigen::Vector3d(
            component.radialX, component.radialY,
            component.position + aggregateLocalCg(component));
        if (component.aerodynamic) {
            cnaTotal += component.cna;
            cpMoment += component.cna * component.cp;
        }
    }
    Eigen::Vector3d cg = Eigen::Vector3d::Zero();
    if (totalMass > 0.0) {
        cg = firstMoment / totalMass;
    }
    double cp = std::abs(cnaTotal) > 1e-12 ? cpMoment / cnaTotal : 0.0;
    
    const barrowman_drag::Model dragModel =
        barrowman_drag::build(physicalInput, referenceDiameter);

    using namespace RoketSiM::AeroExtensions;
    const AeroExtensionConfiguration extensionConfiguration =
        aeroExtensionConfiguration(input);
    const bool extensionEvaluationRequested =
        anyAeroExtensionEnabled(extensionConfiguration);
    RocketGeometry extensionGeometry;
    extensionGeometry.referenceAreaM2 = dragModel.referenceArea;
    extensionGeometry.referenceLengthM = referenceDiameter;
    extensionGeometry.referenceDiameterM = referenceDiameter;
    extensionGeometry.totalLengthM = totalLength;
    extensionGeometry.components.reserve(components.size());
    for (const auto& component : components) {
        RocketGeometryComponent item;
        item.id = component.id;
        item.type = component.type;
        item.axialPositionM = component.position;
        item.lengthM = component.length;
        item.radialXM = component.radialX;
        item.radialYM = component.radialY;
        item.scalarProperties = {
            {"cna_per_rad", component.cna},
            {"cp_m", component.cp},
            {"fin_planform_area_m2", component.finArea},
            {"fin_span_m", component.finSpan},
            {"fin_mac_m", component.finMacLength},
            {"fin_body_radius_m", component.finBodyRadius},
            {"fin_count", static_cast<double>(component.finCount)},
        };
        extensionGeometry.components.push_back(std::move(item));
    }
        
    double targetMach = input.contains("target_mach") ? input["target_mach"].get<double>() : 0.0;
    double targetAoa = input.contains("target_aoa") ? input["target_aoa"].get<double>() : 0.0;
    
    constexpr double seaLevelSpeedOfSound = 340.294;
    constexpr double seaLevelKinematicViscosity = 1.4607e-5;
    
    barrowman_drag::Result targetDrag;
    if (targetMach > 0.0) {
        const double reynolds = targetMach * seaLevelSpeedOfSound
            * dragModel.aerodynamicLength / seaLevelKinematicViscosity;
        targetDrag = barrowman_drag::calculate(dragModel, targetMach, reynolds, targetAoa);

        // Return the design state at the requested Mach, not the legacy
        // Mach-zero component sum.  FinSetCalc's Barrowman CNa and CP are
        // Mach-dependent, therefore the aggregate must use the same values
        // as the per-component breakdown below.
        cnaTotal = 0.0;
        cpMoment = 0.0;
        for (const auto& component : components) {
            if (!component.aerodynamic) continue;
            double componentCna = component.cna;
            double componentCp = component.cp;
            if (component.fin) {
                componentCna = finCnaAtMach(
                    component, targetMach, dragModel.referenceArea, targetAoa);
                componentCp = component.position + component.finMacLead
                    + finCpFraction(component, targetMach) * component.finMacLength;
            } else if (component.tubeFin) {
                componentCp = component.position + tubeFinCpFraction(
                    targetMach, component.tubeFinAspectRatio) * component.length;
            }
            cnaTotal += componentCna;
            cpMoment += componentCna * componentCp;
        }
        cp = std::abs(cnaTotal) > 1e-12 ? cpMoment / cnaTotal : 0.0;
    }

    const auto evaluateExtensions = [&](double mach, double aoa,
                                        double baselineCna, double baselineCp,
                                        double baselineCd) {
        AeroState state;
        state.mach = mach;
        state.alphaRad = aoa;
        state.velocityMS = mach * seaLevelSpeedOfSound;
        state.densityKgM3 = 1.225;
        state.dynamicPressurePa = 0.5 * state.densityKgM3
            * state.velocityMS * state.velocityMS;
        state.referenceAreaM2 = dragModel.referenceArea;
        state.referenceLengthM = referenceDiameter;
        state.reynoldsReferenceLengthM = dragModel.aerodynamicLength;
        state.airVelocityBodyMS = {0.0, 0.0, state.velocityMS};
        state.reynolds = state.velocityMS * dragModel.aerodynamicLength
            / seaLevelKinematicViscosity;
        BaselineAeroResult baseline;
        baseline.coefficients.drag = baselineCd;
        baseline.coefficients.normalForceSlopePerRad = baselineCna;
        baseline.coefficients.normalForceSlopeMomentMPerRad = baselineCna * baselineCp;
        return extensionPipeline.evaluate(
            extensionGeometry, state, baseline,
            AeroExtensionContext{"internal", "design_analysis"},
            extensionConfiguration);
    };

    const AeroExtensionEvaluation targetExtensionEvaluation = evaluateExtensions(
        targetMach, targetAoa, cnaTotal, cp, targetDrag.totalCd);
    if (extensionEvaluationRequested) {
        cnaTotal = targetExtensionEvaluation.finalResult.coefficients
            .normalForceSlopePerRad;
        cpMoment = targetExtensionEvaluation.finalResult.coefficients
            .normalForceSlopeMomentMPerRad;
        cp = std::abs(cnaTotal) > 1e-12 ? cpMoment / cnaTotal : 0.0;
        targetDrag.totalCd = targetExtensionEvaluation.finalResult.coefficients.drag;
    }

    Eigen::Matrix3d inertia = Eigen::Matrix3d::Zero();
    json breakdown = json::array();
    const auto dragInstanceCount = [&](const ComponentResult& component) {
        if (component.type == "Fins" || component.type == "Freeform Fin Set") {
            return std::max(component.finCount, 1);
        }
        for (const auto& tube : dragModel.tubeComponents) {
            if (tube.id == component.id) return std::max(tube.count, 1);
        }
        for (const auto& button : dragModel.railButtonComponents) {
            if (button.id == component.id) return std::max(button.count, 1);
        }
        for (const auto& tubeFin : dragModel.tubeFinComponents) {
            if (tubeFin.id == component.id) return std::max(tubeFin.count, 1);
        }
        return 1;
    };
    for (const auto& component : components) {
        const Eigen::Vector3d absoluteCg(
            component.radialX, component.radialY,
            component.position + aggregateLocalCg(component));
        const Eigen::Matrix3d& localInertia = component.inertiaLocal;
        const Eigen::Vector3d displacement = absoluteCg - cg;
        inertia += localInertia + component.mass * (
            displacement.squaredNorm() * Eigen::Matrix3d::Identity()
            - displacement * displacement.transpose());
        
        double componentCna = component.cna;
        double componentCp = component.cp;
        if (targetMach > 0.0 && component.aerodynamic) {
            if (component.fin) {
                componentCna = finCnaAtMach(
                    component, targetMach, dragModel.referenceArea, targetAoa);
                componentCp = component.position + component.finMacLead
                    + finCpFraction(component, targetMach) * component.finMacLength;
            } else if (component.tubeFin) {
                componentCp = component.position + tubeFinCpFraction(
                    targetMach, component.tubeFinAspectRatio) * component.length;
            }
        }
        
        double frictionCd = 0.0, pressureCd = 0.0, baseCd = 0.0, totalCd = 0.0;
        if (targetMach > 0.0 && targetDrag.components.count(component.id)) {
            const auto& cd = targetDrag.components.at(component.id);
            frictionCd = cd.frictionCd;
            pressureCd = cd.pressureCd;
            baseCd = cd.baseCd;
            totalCd = cd.totalCd;
        }
        const int dragCount = dragInstanceCount(component);
        // OpenRocket's Component Analysis presents a FinSet's aerodynamic
        // columns per fin, while its Total column remains the whole set.  It
        // also groups the fin trailing-edge term with pressure drag.  Keep the
        // canonical cd_* fields below as the aggregate physical decomposition.
        const double displayPressureCd = component.fin
            ? (pressureCd + baseCd) / dragCount : pressureCd;
        const double displayBaseCd = component.fin ? 0.0 : baseCd;
        const double displayFrictionCd = component.fin
            ? frictionCd / dragCount : frictionCd;
        const double displayPerInstanceCd = totalCd / dragCount;

        json clusterOffsets = json::array();
        for (const auto& offset : component.motorClusterOffsets) {
            clusterOffsets.push_back({offset.x(), offset.y()});
        }
        breakdown.push_back({
            {"id", component.id},
            {"type", component.type},
            {"position_m", component.position},
            {"radial_position_m", {component.radialX, component.radialY}},
            {"angle_deg", component.angleDeg},
            {"length_m", component.length},
            {"volume_m3", component.volume},
            {"mass_kg", component.mass},
            {"local_cg_m", component.localCg},
            {"aggregate_local_cg_m", aggregateLocalCg(component)},
            {"instance_axial_offsets_m", component.axialInstanceOffsets},
            {"absolute_cg_m", absoluteCg.z()},
            {"absolute_cg_xyz_m", {absoluteCg.x(), absoluteCg.y(), absoluteCg.z()}},
            {"inertia_transverse_kgm2", component.iTransverse},
            {"inertia_axial_kgm2", component.iAxial},
            {"inertia_tensor_local_kgm2", {
                {component.inertiaLocal(0, 0), component.inertiaLocal(0, 1),
                 component.inertiaLocal(0, 2)},
                {component.inertiaLocal(1, 0), component.inertiaLocal(1, 1),
                 component.inertiaLocal(1, 2)},
                {component.inertiaLocal(2, 0), component.inertiaLocal(2, 1),
                 component.inertiaLocal(2, 2)}}},
            {"aerodynamic", component.aerodynamic},
            {"cna", componentCna},
            {"cp_m", componentCp},
            {"fin_geometry_model", component.fin
                ? (component.type == "Freeform Fin Set"
                    ? "exact_polygon" : "trapezoidal") : ""},
            {"fin_planform_area_m2", component.finArea},
            {"fin_span_m", component.finSpan},
            {"fin_mac_m", component.finMacLength},
            {"fin_mid_chord_sweep_cosine", component.finCosGamma},
            {"motor", component.motor},
            {"motor_dry_mass_kg", component.motorDryMass
                * component.motorCluster.count},
            {"motor_propellant_mass_kg", component.motorPropellantMass
                * component.motorCluster.count},
            {"motor_dry_mass_per_motor_kg", component.motorDryMass},
            {"motor_propellant_mass_per_motor_kg",
                component.motorPropellantMass},
            {"motor_designation", component.motorDesignation},
            {"motor_manufacturer", component.motorManufacturer},
            {"motor_case_info", component.motorCaseInfo},
            {"motor_propellant_info", component.motorPropellantInfo},
            {"motor_delays", component.motorDelays},
            {"motor_sparky", component.motorSparky},
            {"motor_available", component.motorAvailable},
            {"motor_cluster_layout", motorClusterLayoutName(
                component.motorCluster.layout)},
            {"motor_cluster_count", component.motorCluster.count},
            {"motor_cluster_radius_m", component.motorCluster.radius},
            {"motor_cluster_offsets_m", clusterOffsets},
            {"cd_friction", frictionCd},
            {"cd_pressure", pressureCd},
            {"cd_base", baseCd},
            {"cd_total", totalCd},
            {"drag_instance_count", dragCount},
            {"cd_friction_per_instance", frictionCd / dragCount},
            {"cd_pressure_per_instance", pressureCd / dragCount},
            {"cd_base_per_instance", baseCd / dragCount},
            {"cd_total_per_instance", displayPerInstanceCd},
            {"component_analysis_display", {
                {"pressure_cd", displayPressureCd},
                {"base_cd", displayBaseCd},
                {"friction_cd", displayFrictionCd},
                {"per_instance_cd", displayPerInstanceCd},
                {"total_cd", totalCd},
            }}
            ,{"drag_debug", {
                {"mach", targetMach},
                {"reference_area_m2", dragModel.referenceArea},
                {"nose_length_m", component.type == "Nose Cone" ? component.length : 0.0},
                {"nose_base_radius_m", component.noseBaseRadius},
                {"shape", component.noseShape},
                {"shape_parameter", component.noseShapeParameter},
                {"pressure_cd", pressureCd},
                {"friction_cd", frictionCd},
                {"base_cd", baseCd},
                {"total_cd", totalCd},
            }}
        });
    }
    // Match OpenRocket's category presentation without changing physical
    // drag: fin trailing-edge/base drag is displayed as pressure drag.
    double displayPressureCd = targetDrag.pressureCd;
    double displayBaseCd = targetDrag.baseCd;
    for (const auto& component : breakdown) {
        if (component.value("fin_geometry_model", std::string{}).empty()) continue;
        const double finBaseCd = component.value("cd_base", 0.0);
        displayPressureCd += finBaseCd;
        displayBaseCd -= finBaseCd;
    }
    json machCurve = json::array();
    json cnaCurve = json::array();
    json cpCurve = json::array();
    json cdCurve = json::array();
    for (int i = 0; i < 120; ++i) {
        const double mach = 0.1 + (2.5 - 0.1) * i / 119.0;
        double dynamicCna = 0.0;
        double dynamicMoment = 0.0;
        for (const auto& component : components) {
            if (!component.aerodynamic) continue;
            double componentCna = component.cna;
            double componentCp = component.cp;
            if (component.fin) {
                componentCna = finCnaAtMach(
                    component, mach, dragModel.referenceArea, targetAoa);
                componentCp = component.position + component.finMacLead
                    + finCpFraction(component, mach) * component.finMacLength;
            } else if (component.tubeFin) {
                componentCp = component.position + tubeFinCpFraction(
                    mach, component.tubeFinAspectRatio) * component.length;
            }
            dynamicCna += componentCna;
            dynamicMoment += componentCna * componentCp;
        }
        const double dynamicCp = std::abs(dynamicCna) > 1e-12
            ? dynamicMoment / dynamicCna : cp;
        constexpr double seaLevelSpeedOfSound = 340.294;
        constexpr double seaLevelKinematicViscosity = 1.4607e-5;
        const double reynolds = mach * seaLevelSpeedOfSound
            * dragModel.aerodynamicLength / seaLevelKinematicViscosity;
        const double baselineCd = barrowman_drag::calculate(
            dragModel, mach, reynolds, targetAoa).totalCd;
        const AeroExtensionEvaluation curveExtensionEvaluation = evaluateExtensions(
            mach, targetAoa, dynamicCna, dynamicCp, baselineCd);
        double finalCp = dynamicCp;
        double cd = baselineCd;
        if (extensionEvaluationRequested) {
            dynamicCna = curveExtensionEvaluation.finalResult.coefficients
                .normalForceSlopePerRad;
            dynamicMoment = curveExtensionEvaluation.finalResult.coefficients
                .normalForceSlopeMomentMPerRad;
            finalCp = std::abs(dynamicCna) > 1e-12
                ? dynamicMoment / dynamicCna : dynamicCp;
            cd = curveExtensionEvaluation.finalResult.coefficients.drag;
        }
        machCurve.push_back(mach);
        cnaCurve.push_back(dynamicCna);
        cpCurve.push_back(finalCp);
        cdCurve.push_back(cd);
    }

    return {
        {"mass_kg", totalMass},
        {"cg_m", {cg.x(), cg.y(), cg.z()}},
        {"inertia_kgm2", {
            {inertia(0, 0), inertia(0, 1), inertia(0, 2)},
            {inertia(1, 0), inertia(1, 1), inertia(1, 2)},
            {inertia(2, 0), inertia(2, 1), inertia(2, 2)}
        }},
        {"reference_diameter_m", referenceDiameter},
        {"cna", cnaTotal},
        {"cp_m", cp},
        {"stability_margin_cal", referenceDiameter > 0.0
            ? (cp - cg.z()) / referenceDiameter : 0.0},
        {"total_length_m", totalLength},
        {"mach_curve", machCurve},
        {"cna_curve", cnaCurve},
        {"cp_curve", cpCurve},
        {"cd_curve", cdCurve},
        {"drag", {
            {"conditions", {
                {"mach", targetMach},
                {"velocity_m_s", targetMach * seaLevelSpeedOfSound},
                {"density_kg_m3", 1.225},
                {"dynamic_viscosity_pa_s", 1.225 * seaLevelKinematicViscosity},
                {"reference_diameter_m", referenceDiameter},
                {"reference_area_m2", dragModel.referenceArea},
                {"reynolds", targetDrag.reynolds},
            }},
            {"rocket", {
                {"pressure_cd", targetDrag.pressureCd},
                {"base_cd", targetDrag.baseCd},
                {"friction_cd", targetDrag.frictionCd},
                {"total_cd", targetDrag.totalCd},
            }},
            {"component_analysis_display", {
                {"pressure_cd", displayPressureCd},
                {"base_cd", displayBaseCd},
                {"friction_cd", targetDrag.frictionCd},
                {"total_cd", targetDrag.totalCd},
            }},
        }},
        {"drag_model", barrowman_drag::toJson(dragModel)},
        {"aero_extensions", aeroExtensionEvaluationJson(targetExtensionEvaluation)},
        {"assemblies", physicalInput.value("physical_assemblies", json::array())},
        {"components", breakdown}
    };
}

inline json analyze(const json& rawInput) {
    static const RoketSiM::AeroExtensions::AeroExtensionPipeline noExtensions;
    return analyze(rawInput, noExtensions);
}

}  // namespace design
