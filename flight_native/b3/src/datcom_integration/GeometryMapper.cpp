#include "datcom_integration/MappedGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <unordered_map>

namespace RoketSiM::DatcomIntegration {
namespace {

using json = nlohmann::json;

double number(const json& value, const char* key, double fallback = 0.0) {
    const auto found = value.find(key);
    return found != value.end() && found->is_number() ? found->get<double>() : fallback;
}

std::string text(const json& value, const char* key, const char* fallback = "") {
    const auto found = value.find(key);
    return found != value.end() && found->is_string() ? found->get<std::string>() : fallback;
}

const json& params(const json& component) {
    static const json empty = json::object();
    const auto found = component.find("params");
    return found != component.end() && found->is_object() ? *found : empty;
}

std::optional<ExactFinPlanform> finPlanform(const std::string& type, const json& p, double origin) {
    double root = number(p, "root_chord"), tip = number(p, "tip_chord"), span = number(p, "span");
    double rootX = 0, tipX = number(p, "sweep_length");
    bool valid = type == "Fins";
    if (type == "Freeform Fin Set") {
        std::vector<std::pair<double, double>> points;
        const auto raw = p.find("source_points_m");
        if (raw != p.end() && raw->is_array()) {
            for (const auto& point : *raw) {
                if (!point.is_array() || point.size() != 2 || !point[0].is_number() || !point[1].is_number())
                    return std::nullopt;
                points.emplace_back(point[0].get<double>(), point[1].get<double>());
            }
        } else {
            std::istringstream stream(text(p, "source_points")); std::string token;
            while (std::getline(stream, token, ';')) {
                std::istringstream pair(token); double x, y; char comma;
                if (!(pair >> x >> comma >> y) || comma != ',') return std::nullopt;
                pair >> std::ws;
                if (!pair.eof()) return std::nullopt;
                points.emplace_back(x, y);
            }
        }
        if (points.size() == 5 && points.front() == points.back()) points.pop_back();
        if (points.size() != 4) return std::nullopt;
        span = 0;
        for (const auto& [x, y] : points) {
            if (!std::isfinite(x) || !std::isfinite(y) || y < 0) return std::nullopt;
            span = std::max(span, y);
        }
        std::vector<double> roots, tips;
        double twiceArea = 0;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const auto [x, y] = points[i]; const auto [nx, ny] = points[(i+1)%4];
            twiceArea += x*ny-nx*y;
            if (y == 0) roots.push_back(x); else if (y == span) tips.push_back(x);
        }
        if (roots.size() != 2 || tips.size() != 2) return std::nullopt;
        std::sort(roots.begin(), roots.end()); std::sort(tips.begin(), tips.end());
        root = roots[1]-roots[0]; tip = tips[1]-tips[0]; rootX = roots[0]; tipX = tips[0];
        const double area = (root+tip)*span;
        valid = area > 0 && std::abs(std::abs(twiceArea)-area) <= area*1e-12;
    }
    valid = valid && std::isfinite(root) && std::isfinite(tip) && std::isfinite(span)
        && std::isfinite(rootX) && std::isfinite(tipX) && root > 0 && tip >= 0 && span > 0;
    if (!valid || !std::isfinite(origin+rootX) || !std::isfinite(origin+tipX)) return std::nullopt;
    return ExactFinPlanform{root, tip, span, {origin+rootX, origin+tipX},
        type == "Freeform Fin Set" ? FinPlanformSource::FreeformSourceOutline
                                  : FinPlanformSource::ConventionalScalars};
}

json planformToJson(const std::optional<ExactFinPlanform>& planform) {
    if (!planform) return {{"status", "NOT_REPRESENTABLE"}};
    const auto& p = *planform;
    return {{"status", "REPRESENTABLE"},
            {"span_stations_from_root_m", json::array({0.0, p.spanM})},
            {"chord_stations_m", json::array({p.rootChordM, p.tipChordM})},
            {"xle_stations_m", p.leadingEdgeM},
            {"geometry_source", p.source == FinPlanformSource::FreeformSourceOutline
                ? "freeform_source_outline" : "conventional_scalars"}};
}

double componentLength(const json& component) {
    const auto& p = params(component);
    const auto type = text(component, "type");
    if (type == "Fins") return number(p, "root_chord");
    if (type == "Freeform Fin Set") {
        // Reverse-from-tail and stacked placement must not use stale imported
        // root_chord either. Preserve the canonical root-chord length meaning.
        const auto planform = finPlanform(type, p, 0.0);
        return planform ? planform->rootChordM : 0.0;
    }
    if (type == "Rail Button") return number(p, "height");
    if (type == "Centering Ring" || type == "Bulkhead") return number(p, "thickness");
    return number(p, "length");
}

struct Located { const json* source; double positionM; };

std::vector<Located> locate(const json& components, std::vector<Diagnostic>& diagnostics) {
    std::unordered_map<std::string, const json*> byId;
    std::unordered_map<std::string, std::vector<const json*>> children;
    for (const auto& component : components) {
        const auto id = text(component, "id");
        if (id.empty()) {
            diagnostics.push_back({"DATCOM_COMPONENT_ID_REQUIRED", DiagnosticSeverity::Error, std::nullopt,
                "Every mapped component requires a stable source id.", DiagnosticReason::InvalidRequest});
            continue;
        }
        if (!byId.emplace(id, &component).second) {
            diagnostics.push_back({"DATCOM_DUPLICATE_COMPONENT_ID", DiagnosticSeverity::Error, std::nullopt,
                "Duplicate component id '" + id + "'.", DiagnosticReason::InvalidRequest});
        }
    }
    for (const auto& component : components) {
        const auto parent = text(component, "parent_id");
        children[byId.count(parent) ? parent : "__rocket__"].push_back(&component);
    }
    std::vector<Located> result;
    std::vector<std::string> visiting;
    const std::function<double(const std::string&, double, double)> resolve =
        [&](const std::string& parent, double origin, double parentLength) -> double {
        if (std::find(visiting.begin(), visiting.end(), parent) != visiting.end()) {
            diagnostics.push_back({"DATCOM_COMPONENT_CYCLE", DiagnosticSeverity::Error, std::nullopt,
                "Component parent_id cycle detected.", DiagnosticReason::InvalidRequest});
            return 0.0;
        }
        const auto found = children.find(parent);
        if (found == children.end()) return 0.0;
        visiting.push_back(parent);
        double sequentialTail = 0.0, furthestTail = 0.0;
        std::vector<const json*> reverse;
        for (const auto* component : found->second) {
            const auto mode = text(*component, "pos_mode", "Automatic (Stacked)");
            if (mode == "Reverse from Tail") { reverse.push_back(component); continue; }
            double length = componentLength(*component);
            const double offset = number(*component, "pos_value");
            const double local = mode == "Automatic (Stacked)" ? sequentialTail
                : mode == "Relative Offset" ? sequentialTail + offset : offset;
            const double position = origin + local;
            if (text(*component, "type") != "Stage Separator") result.push_back({component, position});
            const double childExtent = resolve(text(*component, "id"), position, length);
            if (text(*component, "type") == "Stage Separator") length = childExtent;
            furthestTail = std::max(furthestTail, local + length);
            if (mode == "Automatic (Stacked)" || mode == "Relative Offset") sequentialTail = local + length;
        }
        const double tail = parentLength > 0.0 ? parentLength : furthestTail;
        for (const auto* component : reverse) {
            const double length = componentLength(*component);
            const double position = origin + tail - length - number(*component, "pos_value");
            if (text(*component, "type") != "Stage Separator") result.push_back({component, position});
            resolve(text(*component, "id"), position, length);
            furthestTail = std::max(furthestTail, tail - number(*component, "pos_value"));
        }
        visiting.pop_back();
        return furthestTail;
    };
    resolve("__rocket__", 0.0, 0.0);
    // Root traversal already visits stage descendants. A second stage pass
    // duplicated every imported finset and nose. Preserve source-list order.
    std::unordered_map<const json*, std::size_t> sourceOrder;
    for (std::size_t i = 0; i < components.size(); ++i) sourceOrder[&components[i]] = i;
    std::stable_sort(result.begin(), result.end(), [&](const Located& a, const Located& b) {
        return sourceOrder[a.source] < sourceOrder[b.source];
    });
    return result;
}

BodyFamily bodyFamily(const json& component) {
    const auto& p = params(component);
    const auto family = text(component, "sub_type", text(p, "shape", "Unknown").c_str());
    if (family == "Conical" || family == "Cone") return BodyFamily::SharpCone;
    if (family == "Tangent Ogive") return BodyFamily::TangentOgive;
    if (family == "Parabolic") return BodyFamily::Parabolic;
    if (family == "Power" || family == "Power Series") return BodyFamily::Power;
    if (family == "Von Karman" || family == "von Karman" || family == "von Kármán") return BodyFamily::VonKarman;
    if (family == "Haack") return BodyFamily::Haack;
    if (family == "Secant Ogive") return BodyFamily::SecantOgive;
    if (family == "Elliptical" || family == "Ellipsoid") return BodyFamily::Elliptical;
    if (family == "Option2" || family == "Option-2" || family == "Raw Coordinates") return BodyFamily::Option2;
    return BodyFamily::Unknown;
}

struct BodyIdentity {
    BodyFamily family = BodyFamily::Unknown;
    std::string sourceFamily;
    std::optional<double> shapeParameter;
    std::string shapeParameterSource;
};

std::optional<double> numericField(const json& value, const char* key) {
    const auto found = value.find(key);
    if (found == value.end() || !found->is_number()) return std::nullopt;
    return found->get<double>();
}

BodyIdentity bodyIdentity(const json& component) {
    const auto& p = params(component);
    const auto canonicalFamily = text(component, "sub_type", text(p, "shape", "Unknown").c_str());
    BodyIdentity result;
    result.family = bodyFamily(component);
    result.sourceFamily = text(p, "source_shape", canonicalFamily.c_str());
    result.shapeParameterSource = text(p, "shape_parameter_source");
    result.shapeParameter = numericField(p, "shape_parameter");
    if (!result.shapeParameter) {
        const char* legacyKey = result.family == BodyFamily::Parabolic ? "K"
            : result.family == BodyFamily::Power ? "n"
            : (result.family == BodyFamily::Haack || result.family == BodyFamily::VonKarman) ? "C"
            : nullptr;
        if (legacyKey) result.shapeParameter = numericField(p, legacyKey);
        if (result.shapeParameter && result.shapeParameterSource.empty()) {
            result.shapeParameterSource = "LEGACY_EXPLICIT";
        }
    }
    if (result.shapeParameterSource == "EXPLICIT_INVALID") result.shapeParameter.reset();

    if (!result.shapeParameter && result.shapeParameterSource.empty()) {
        if (result.family == BodyFamily::SecantOgive) {
            result.shapeParameterSource = "ABSENT_UNRECOVERABLE";
        } else if (result.family == BodyFamily::TangentOgive) result.shapeParameter = 1.0;
        else if (result.family == BodyFamily::Parabolic) result.shapeParameter = 0.5;
        else if (result.family == BodyFamily::Power) result.shapeParameter = 0.5;
        else if (result.family == BodyFamily::Haack || result.family == BodyFamily::VonKarman) result.shapeParameter = 0.0;
        if (result.shapeParameterSource.empty()) {
            result.shapeParameterSource = result.shapeParameter ? "LEGACY_DEFAULT" : "NOT_APPLICABLE";
        }
    }
    if (result.family == BodyFamily::Haack && result.shapeParameter
        && std::isfinite(*result.shapeParameter) && std::abs(*result.shapeParameter) <= 1e-12) {
        result.family = BodyFamily::VonKarman;
    }
    return result;
}

bool isInternal(const std::string& type) {
    static const std::vector<std::string> values{
        "Centering Ring", "Bulkhead", "Inner Tube", "Coupler", "Internal Mass", "Avionics",
        "Motor", "Motor Mount", "Engine Block", "Motor Retainer", "Parachute", "Shock Cord",
        "Recovery Hardware", "Stage Separator"};
    return std::find(values.begin(), values.end(), type) != values.end();
}

}  // namespace

std::string toString(BodyFamily value) {
    switch (value) {
    case BodyFamily::Missing: return "MISSING";
    case BodyFamily::SharpCone: return "SHARP_CONE";
    case BodyFamily::TangentOgive: return "TANGENT_OGIVE";
    case BodyFamily::Parabolic: return "PARABOLIC";
    case BodyFamily::Power: return "POWER";
    case BodyFamily::Haack: return "HAACK";
    case BodyFamily::VonKarman: return "VON_KARMAN";
    case BodyFamily::SecantOgive: return "SECANT_OGIVE";
    case BodyFamily::Elliptical: return "ELLIPTICAL";
    case BodyFamily::Option2: return "OPTION2";
    case BodyFamily::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

std::string toString(AnalysisMode value) {
    switch (value) {
    case AnalysisMode::StaticLongitudinal: return "STATIC_LONGITUDINAL";
    case AnalysisMode::StaticDerivatives: return "STATIC_DERIVATIVES";
    case AnalysisMode::DynamicDerivatives: return "DYNAMIC_DERIVATIVES";
    case AnalysisMode::MagnusSpin: return "MAGNUS_SPIN";
    case AnalysisMode::TrimPitch: return "TRIM_PITCH";
    case AnalysisMode::HypersBodyOnly: return "HYPERS_BODY_ONLY";
    case AnalysisMode::BasePlume: return "BASE_PLUME";
    case AnalysisMode::Inlet: return "INLET";
    }
    return "STATIC_LONGITUDINAL";
}

AnalysisMode analysisModeFromString(const std::string& value) {
    if (value == "STATIC_DERIVATIVES") return AnalysisMode::StaticDerivatives;
    if (value == "DYNAMIC_DERIVATIVES") return AnalysisMode::DynamicDerivatives;
    if (value == "MAGNUS_SPIN") return AnalysisMode::MagnusSpin;
    if (value == "TRIM_PITCH") return AnalysisMode::TrimPitch;
    if (value == "HYPERS_BODY_ONLY" || value == "HYPERS") return AnalysisMode::HypersBodyOnly;
    if (value == "BASE_PLUME") return AnalysisMode::BasePlume;
    if (value == "INLET") return AnalysisMode::Inlet;
    return AnalysisMode::StaticLongitudinal;
}

MappedRequest mapGeometryRequest(const nlohmann::json& input) {
    MappedRequest out;
    const auto sourceIt = input.find("geometry_components");
    const auto componentsIt = input.find("components");
    const json* components = sourceIt != input.end() && sourceIt->is_array() ? &*sourceIt
        : componentsIt != input.end() && componentsIt->is_array() ? &*componentsIt : nullptr;
    if (!components) {
        out.diagnostics.push_back({"DATCOM_COMPONENTS_REQUIRED", DiagnosticSeverity::Error, std::nullopt,
            "DATCOM Equivalent requires a canonical components array.", DiagnosticReason::InvalidRequest});
        return out;
    }

    const auto located = locate(*components, out.diagnostics);
    for (const auto& item : located) {
        const auto& component = *item.source;
        const auto& p = params(component);
        const auto type = text(component, "type");
        const auto id = text(component, "id");
        if (type == "Nose Cone" || type == "Axisymmetric Body" || type == "Freeform Body") {
            if (out.body.family != BodyFamily::Missing) {
                out.diagnostics.push_back({"DATCOM_MULTIPLE_NOSES", DiagnosticSeverity::Error, std::nullopt,
                    "Exactly one primary BODY nose is required for the compatibility operation.", DiagnosticReason::InvalidRequest});
                out.unsupportedComponents.push_back({id, type, component});
                continue;
            }
            const auto identity = type == "Nose Cone" ? bodyIdentity(component)
                : BodyIdentity{BodyFamily::Option2, type, std::nullopt, "NOT_APPLICABLE"};
            out.body = {id, text(component, "source_component_id"), identity.sourceFamily,
                identity.shapeParameterSource, identity.family, identity.shapeParameter,
                item.positionM, number(p, "length"), number(p, "diameter", number(p, "outer_diameter")), p};
        } else if (type == "Body Tube") {
            const double diameter = number(p, "diameter", number(p, "outer_diameter"));
            out.bodySections.push_back({id, type, "CONSTANT_CYLINDER", item.positionM,
                number(p, "length"), diameter, diameter, p});
        } else if (type == "Transition") {
            const auto family = text(p, "shape", "Conical");
            out.bodySections.push_back({id, type, family == "Ogival" ? "OGIVAL" : "CONICAL",
                item.positionM, number(p, "length"), number(p, "fore_diameter"),
                number(p, "aft_diameter"), p});
        } else if (type == "Fins" || type == "Freeform Fin Set") {
            const auto sourceProfile = text(p, "cross_section", "unknown");
            const auto section = finSectionFromString(sourceProfile);
            const auto support = finSectionSupport(section);
            const std::string mappedProfile = support == FinSectionSupport::Exact ? "HEX" : "";
            out.finsets.push_back({id, item.positionM, number(p, "root_chord"), number(p, "tip_chord"),
                number(p, "span"), number(p, "sweep_length"), number(p, "thickness"),
                static_cast<int>(std::lround(number(p, "n_fins", number(p, "n", number(p, "fin_count"))))),
                number(component, "angle_deg"), number(p, "cant_angle_deg"), sourceProfile, mappedProfile, p, section, support, text(component, "source_component_id"), finPlanform(type, p, item.positionM)});
        } else if (type == "Tube Fin Set") {
            out.unsupportedComponents.push_back({id, type, component});
        } else if (type == "Launch Lug" || type == "Rail Button") {
            out.optionalFeatures.push_back({id, type, component});
        } else if (type == "Inlet") {
            out.optionalFeatures.push_back({id, type, component});
        } else if (!isInternal(type)) {
            out.unsupportedComponents.push_back({id, type, component});
        }
    }

    const auto& flight = input.value("flight", json::object());
    const auto& reference = input.value("reference", json::object());
    const auto& analysis = input.value("analysis", json::object());
    const auto mode = text(input, "analysis_mode", text(analysis, "mode", "STATIC_LONGITUDINAL").c_str());
    out.condition.analysisMode = analysisModeFromString(mode);
    out.condition.mach = number(flight, "mach");
    out.condition.alphaDeg = number(flight, "alpha_deg");
    out.condition.betaDeg = number(flight, "beta_deg");
    out.condition.altitudeM = number(flight, "altitude_m", number(input, "altitude_m"));
    out.condition.reynoldsPerM = number(flight, "reynolds_per_m");
    out.condition.boundaryLayer = text(flight, "boundary_layer", "TURB");
    out.condition.roughnessM = number(flight, "roughness_m");
    out.condition.finCantDeg = number(flight, "fin_cant_deg");
    out.condition.controlDeflectionDeg = number(flight, "control_deflection_deg");
    out.reference.areaM2 = number(reference, "area_m2");
    out.reference.lengthM = number(reference, "length_m");
    out.reference.diameterM = number(reference, "diameter_m",
        out.body.diameterM > 0.0 ? out.body.diameterM : (out.bodySections.empty() ? 0.0 : out.bodySections.front().foreDiameterM));
    out.reference.xcgM = number(reference, "xcg_m");
    return out;
}

nlohmann::json mappedRequestToJson(const MappedRequest& value) {
    json sections = json::array(), fins = json::array(), optional = json::array(), unsupported = json::array();
    for (const auto& item : value.bodySections) sections.push_back({
        {"component_id", item.sourceComponentId}, {"type", item.type}, {"family", item.family},
        {"origin_m", item.originM}, {"length_m", item.lengthM},
        {"fore_diameter_m", item.foreDiameterM}, {"aft_diameter_m", item.aftDiameterM},
        {"source_parameters", item.sourceParameters}});
    for (const auto& item : value.finsets) fins.push_back({
        {"component_id", item.sourceComponentId}, {"axial_location_m", item.axialLocationM},
        {"root_chord_m", item.rootChordM}, {"tip_chord_m", item.tipChordM}, {"span_m", item.spanM},
        {"sweep_length_m", item.sweepLengthM}, {"thickness_m", item.thicknessM},
        {"panel_count", item.panelCount}, {"orientation_deg", item.orientationDeg}, {"cant_deg", item.cantDeg},
        {"imported_source_component_id", item.importedSourceComponentId},
        {"source_profile", item.sourceProfile}, {"mapped_profile", item.mappedProfile},
        {"section_id", toString(item.sectionFamily)}, {"section_support", toString(item.sectionSupport)},
        {"planform", planformToJson(item.exactPlanform)},
        {"source_parameters", item.sourceParameters}});
    for (const auto& item : value.optionalFeatures) optional.push_back({
        {"component_id", item.sourceComponentId}, {"type", item.type}, {"source", item.source}});
    for (const auto& item : value.unsupportedComponents) unsupported.push_back({
        {"component_id", item.sourceComponentId}, {"type", item.type}, {"source", item.source}});
    const json bodyParameter = value.body.shapeParameter ? json(*value.body.shapeParameter) : json(nullptr);
    return {
        {"body", {{"component_id", value.body.sourceComponentId},
                  {"imported_source_component_id", value.body.importedSourceComponentId},
                  {"source_family", value.body.sourceFamily}, {"family", toString(value.body.family)},
                  {"shape_parameter", bodyParameter},
                  {"shape_parameter_source", value.body.shapeParameterSource},
                  {"origin_m", value.body.originM}, {"length_m", value.body.lengthM},
                  {"diameter_m", value.body.diameterM}, {"source_parameters", value.body.sourceParameters}}},
        {"body_sections", sections}, {"finsets", fins}, {"optional_features", optional},
        {"unsupported_components", unsupported},
        {"condition", {{"analysis_mode", toString(value.condition.analysisMode)}, {"mach", value.condition.mach},
                       {"alpha_deg", value.condition.alphaDeg}, {"beta_deg", value.condition.betaDeg},
                       {"altitude_m", value.condition.altitudeM}, {"reynolds_per_m", value.condition.reynoldsPerM},
                       {"boundary_layer", value.condition.boundaryLayer}, {"roughness_m", value.condition.roughnessM},
                       {"fin_cant_deg", value.condition.finCantDeg},
                       {"control_deflection_deg", value.condition.controlDeflectionDeg}}},
        {"reference", {{"area_m2", value.reference.areaM2}, {"length_m", value.reference.lengthM},
                       {"diameter_m", value.reference.diameterM}, {"xcg_m", value.reference.xcgM},
                       {"frame_id", value.reference.frameId}}},
    };
}

}  // namespace RoketSiM::DatcomIntegration
