#include "datcom_integration/HexSectionContract.hpp"
#pragma once

// Canonical RoketSiM component-record -> DATCOM Equivalent request adapter.
// This is a whole-provider boundary: it deliberately never calls the
// Barrowman design analyzer or AeroExtensionPipeline.

#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "datcom_equivalent/RawStaticSolver.hpp"
#include "datcom_integration/Integration.hpp"
#include "nlohmann/json.hpp"

namespace datcom_equivalent::roketsim_adapter {

using json = nlohmann::json;

class Unsupported final : public std::runtime_error {
public:
    Unsupported(std::string code, std::string message)
        : std::runtime_error(std::move(message)), code_(std::move(code)) {}
    const std::string& code() const noexcept { return code_; }
private:
    std::string code_;
};

inline double number(const json& object, const char* key, double fallback = 0.0) {
    const auto found = object.find(key);
    if (found == object.end() || !found->is_number()) return fallback;
    return found->get<double>();
}

inline std::string text(const json& object, const char* key, const char* fallback = "") {
    const auto found = object.find(key);
    return found == object.end() || !found->is_string() ? fallback : found->get<std::string>();
}

inline const json& params(const json& component) {
    static const json empty = json::object();
    const auto found = component.find("params");
    return found != component.end() && found->is_object() ? *found : empty;
}

struct LocatedComponent {
    const json* source = nullptr;
    double positionM = 0.0;
};

inline double supportedLength(const json& component) {
    const auto& p = params(component);
    const std::string type = text(component, "type");
    if (type == "Nose Cone" || type == "Body Tube") return number(p, "length");
    if (type == "Fins") return number(p, "root_chord");
    if (type == "Stage Separator") return 0.0;
    // Internal components are geometry-neutral but retain their canonical
    // placement behaviour when an enclosing assembly is resolved.
    if (type == "Internal Mass" || type == "Inner Tube" || type == "Motor"
        || type == "Motor Mount" || type == "Shock Cord") return number(p, "length");
    throw Unsupported("UNSUPPORTED_BODY_GEOMETRY",
        "DATCOM Equivalent rejects exterior component type '" + type + "'.");
}

inline std::vector<LocatedComponent> locateComponents(const json& components) {
    std::unordered_map<std::string, std::vector<const json*>> children;
    std::unordered_map<std::string, const json*> byId;
    std::vector<std::string> stages;
    for (const auto& component : components) {
        const std::string id = text(component, "id");
        if (id.empty()) throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "Every component requires an id.");
        if (!byId.emplace(id, &component).second) {
            throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "Duplicate component id '" + id + "'.");
        }
        if (text(component, "type") == "Stage Separator") stages.push_back(id);
    }
    for (const auto& component : components) {
        const std::string parent = text(component, "parent_id");
        children[byId.count(parent) ? parent : "__rocket__"].push_back(&component);
    }

    std::vector<LocatedComponent> located;
    std::vector<std::string> visiting;
    const auto resolveGroup = [&](auto&& self, const std::string& parent, double origin,
                                  double parentLength) -> double {
        const auto found = children.find(parent);
        if (found == children.end()) return 0.0;
        for (const auto& active : visiting) {
            if (active == parent) throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "Component parent_id cycle.");
        }
        visiting.push_back(parent);
        double sequentialTail = 0.0, furthestTail = 0.0;
        std::vector<const json*> reverse;
        for (const json* component : found->second) {
            const std::string mode = text(*component, "pos_mode", "Automatic (Stacked)");
            if (mode == "Reverse from Tail") { reverse.push_back(component); continue; }
            const double length = supportedLength(*component);
            const double offset = number(*component, "pos_value");
            const double local = mode == "Automatic (Stacked)" ? sequentialTail
                : mode == "Relative Offset" ? sequentialTail + offset : offset;
            const double position = origin + local;
            if (text(*component, "type") != "Stage Separator") located.push_back({component, position});
            self(self, text(*component, "id"), position, length);
            furthestTail = std::max(furthestTail, local + length);
            if (mode == "Automatic (Stacked)" || mode == "Relative Offset") sequentialTail = local + length;
        }
        const double tail = parentLength > 0.0 ? parentLength : furthestTail;
        for (const json* component : reverse) {
            const double length = supportedLength(*component);
            const double position = origin + tail - length - number(*component, "pos_value");
            if (text(*component, "type") != "Stage Separator") located.push_back({component, position});
            self(self, text(*component, "id"), position, length);
            furthestTail = std::max(furthestTail, tail - number(*component, "pos_value"));
        }
        visiting.pop_back();
        return furthestTail;
    };
    resolveGroup(resolveGroup, "__rocket__", 0.0, 0.0);
    double stageOrigin = 0.0;
    for (const std::string& id : stages) stageOrigin += resolveGroup(resolveGroup, id, stageOrigin, 0.0);
    return located;
}

inline std::vector<double> panelAzimuths(int count, double orientationDeg) {
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(count));
    const double first = 180.0 / static_cast<double>(count) + orientationDeg;
    for (int index = 0; index < count; ++index) values.push_back(first + 360.0 * index / count);
    return values;
}

inline DatcomEquivalentRequest mapRequest(const json& input) {
    if (!input.contains("components") || !input["components"].is_array()) {
        throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "DATCOM Equivalent requires canonical components.");
    }
    const auto located = locateComponents(input["components"]);
    const LocatedComponent *nose = nullptr, *body = nullptr, *fin = nullptr;
    for (const auto& item : located) {
        const std::string type = text(*item.source, "type");
        if (type == "Nose Cone") {
            if (nose) throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "Exactly one nose cone is supported.");
            nose = &item;
        } else if (type == "Body Tube") {
            if (body) throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "Exactly one body tube is supported.");
            body = &item;
        } else if (type == "Fins") {
            if (fin) throw Unsupported("MULTIPLE_FINSETS", "DATCOM Equivalent supports exactly one FINSET.");
            fin = &item;
        } else if (type == "Freeform Fin Set") {
            throw Unsupported("UNSUPPORTED_FIN_PROFILE", "Freeform FINSET is not losslessly representable as symmetric HEX.");
        }
    }
    if (!nose) throw Unsupported("UNSUPPORTED_NOSE_FAMILY", "A sharp conical nose is required.");
    if (!body) throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "One constant-diameter body tube is required.");
    if (!fin) throw Unsupported("UNSUPPORTED_FIN_PROFILE", "One supported FINSET is required.");
    const auto& noseParams = params(*nose->source);
    const auto& bodyParams = params(*body->source);
    const auto& finParams = params(*fin->source);
    if (text(*nose->source, "sub_type", "Conical") != "Conical"
        || text(noseParams, "shape", "Conical") != "Conical" || std::abs(nose->positionM) > 1e-9
        || noseParams.value("is_flipped", false)) {
        throw Unsupported("UNSUPPORTED_NOSE_FAMILY", "Only a sharp, nose-forward conical nose is supported.");
    }
    const double diameter = number(bodyParams, "diameter", number(bodyParams, "outer_diameter"));
    const double noseDiameter = number(noseParams, "diameter", diameter);
    const double noseLength = number(noseParams, "length");
    const double bodyLength = number(bodyParams, "length");
    if (!(diameter > 0.0) || !(noseLength > 0.0) || !(bodyLength > 0.0)
        || std::abs(noseDiameter - diameter) > 1e-9 || std::abs(body->positionM - noseLength) > 1e-9) {
        throw Unsupported("UNSUPPORTED_BODY_GEOMETRY", "Geometry must be a contiguous sharp cone plus constant-diameter cylinder.");
    }
    if (diameter < 0.05 || diameter > 0.4) {
        throw Unsupported("DIAMETER_OUT_OF_RANGE", "Body diameter must be within [0.05, 0.4] m.");
    }
    const int panels = static_cast<int>(std::lround(number(finParams, "n_fins", number(finParams, "n"))));
    if (panels != 3 && panels != 4 && panels != 6) throw Unsupported("UNSUPPORTED_PANEL_COUNT", "Supported FINSET counts are 3, 4, or 6.");
    const std::string section = text(finParams, "cross_section");
    if (section != "Diamond" && section != "diamond") {
        throw Unsupported("UNSUPPORTED_FIN_PROFILE", "Only canonical Diamond FINSET sections are supported.");
    }
    const double root = number(finParams, "root_chord");
    const double tip = number(finParams, "tip_chord");
    const double span = number(finParams, "span");
    const double sweep = number(finParams, "sweep_length");
    const double thickness = number(finParams, "thickness");
    if (!(root > 0.0) || !(tip > 0.0) || !(span > 0.0) || !(thickness > 0.0)) {
        throw Unsupported("UNSUPPORTED_FIN_PROFILE", "FINSET root chord, tip chord, span, and thickness must be positive.");
    }
    const json& flight = input.value("flight", json::object());
    const json& reference = input.value("reference", json::object());
    const double mach = number(flight, "mach");
    const double alphaDeg = number(flight, "alpha_deg");
    const double betaDeg = number(flight, "beta_deg");
    const double reynoldsPerM = number(flight, "reynolds_per_m");
    const double roughnessM = number(flight, "roughness_m");
    const std::string boundaryLayer = text(flight, "boundary_layer", "TURB");
    if (mach < 0.2 || mach > 5.0) throw Unsupported("MACH_OUT_OF_RANGE", "Mach must be within [0.2, 5.0].");
    if (alphaDeg < -10.0 || alphaDeg > 10.0) throw Unsupported("ALPHA_OUT_OF_RANGE", "Alpha must be within [-10, 10] degrees.");
    if (betaDeg != 0.0) throw Unsupported("NONZERO_BETA", "DATCOM Equivalent requires beta = 0 degrees.");
    if (number(flight, "fin_cant_deg") != 0.0) throw Unsupported("NONZERO_CANT", "DATCOM Equivalent requires fin cant = 0 degrees.");
    if (number(flight, "control_deflection_deg") != 0.0) throw Unsupported("NONZERO_CONTROL_DEFLECTION", "DATCOM Equivalent requires control deflection = 0 degrees.");
    if (!(reynoldsPerM > 0.0)) throw Unsupported("INVALID_REYNOLDS", "Unit Reynolds must be positive in 1/m.");
    if (roughnessM < 0.0) throw Unsupported("INVALID_ROUGHNESS", "Roughness must be nonnegative metres.");
    if (boundaryLayer != "NATURAL" && boundaryLayer != "TURB") throw Unsupported("UNSUPPORTED_BOUNDARY_LAYER_MODE", "Boundary layer must be NATURAL or TURB.");
    if (!(number(reference, "area_m2") > 0.0)) throw Unsupported("INVALID_REFERENCE_AREA", "Reference area must be positive m².");
    if (!(number(reference, "length_m") > 0.0)) throw Unsupported("INVALID_REFERENCE_LENGTH", "Reference length must be positive m.");
    DatcomEquivalentRequest request;
    request.body = {noseLength, diameter, bodyLength, 0.0};
    request.fin.sspanM = {diameter * 0.5, diameter * 0.5 + span};
    request.fin.chordM = {root, tip};
    request.fin.xleM = {fin->positionM, fin->positionM + sweep};
    RoketSiM::DatcomIntegration::setSharpSymmetricDiamondSection(request.fin, thickness);
    request.fin.panelAzimuthsDeg = panelAzimuths(panels, number(*fin->source, "angle_deg"));
    request.fin.npanel = panels;
    request.flight = {mach, reynoldsPerM, boundaryLayer, roughnessM};
    request.reference = {number(reference, "area_m2"), number(reference, "length_m"), number(reference, "xcg_m")};
    request.alphaDeg = alphaDeg;
    request.betaDeg = betaDeg;
    request.finCantDeg = number(flight, "fin_cant_deg");
    request.controlDeflectionDeg = number(flight, "control_deflection_deg");
    return request;
}

inline json legacyAnalyze(const json& input) {
    try {
        const DatcomEquivalentRequest request = mapRequest(input);
        const DatcomEquivalentResult result = evaluateRawStaticLongitudinal(request);
        const double alpha = request.alphaDeg * 3.14159265358979323846 / 180.0;
        const bool cpDefined = std::abs(result.finalCn) > 1e-8;
        const double cp = cpDefined ? request.reference.xcgM
            - result.finalCm * request.reference.lengthM / result.finalCn : 0.0;
        json panels = json::array();
        for (std::size_t index = 0; index < result.panelCn.size(); ++index) {
            panels.push_back({
                {"panel", static_cast<int>(index + 1)},
                {"azimuth_deg", index < request.fin.panelAzimuthsDeg.size()
                    ? request.fin.panelAzimuthsDeg[index] : 0.0},
                {"equivalent_alpha_deg", index < result.equivalentPanelAnglesDeg.size()
                    ? result.equivalentPanelAnglesDeg[index] : 0.0},
                {"cn", result.panelCn[index]},
                // The finite-alpha owner supplies this panel-normal contribution
                // directly; it is serialized here without a presentation-layer
                // recomputation or any change to the validated solver.
                {"projected_contribution", result.panelCn[index]},
            });
        }
        const json breakdown = {
            {"body", {{"ca_total", result.bodyCa}, {"cn_total", result.bodyCn},
                      {"cm_total", result.bodyCm}}},
            {"fin", {{"ca", result.isolatedFinCa}, {"cna_per_rad", result.isolatedFinCna},
                     {"cn", result.isolatedFinCn}, {"cm", result.isolatedFinCm},
                     {"cp_m", result.isolatedFinCp ? json(*result.isolatedFinCp) : json(nullptr)}}},
            {"body_fin", {{"kw_base", result.kwBase}, {"kw_alpha", result.kw},
                           {"kb", result.kb}, {"kkw", result.kkw}, {"kkb", result.kkb},
                           {"bdwk", result.bdwk}, {"finset_in_body_cn", result.finInBodyCn},
                           {"carryover_cn", result.carryoverCn}, {"carryover_cm", result.carryoverCm}}},
            {"panels", panels},
        };
        return {
            {"provider", "datcom_equivalent"}, {"display_name", "DATCOM Equivalent"},
            {"validity", "supported"}, {"ca", result.finalCa}, {"cn", result.finalCn},
            {"cm", result.finalCm}, {"cp_m", cpDefined ? json(cp) : json(nullptr)},
            {"cl", result.finalCn * std::cos(alpha) - result.finalCa * std::sin(alpha)},
            {"cd", result.finalCa * std::cos(alpha) + result.finalCn * std::sin(alpha)},
            {"method", result.activeBodyMethod}, {"body_fallback", result.bodyFallback},
            {"warnings", result.warnings}, {"component_breakdown", breakdown},
            {"mapped_geometry", {{"body", {{"family", "Sharp Cone + Cylinder"},
                                             {"diameter_m", request.body.diameterM},
                                             {"nose_length_m", request.body.noseLengthM},
                                             {"cylinder_length_m", request.body.centerLengthM}}},
                                 {"finset", {{"count", request.fin.npanel},
                                               {"root_chord_m", request.fin.chordM[0]},
                                               {"tip_chord_m", request.fin.chordM[1]},
                                               {"span_m", request.fin.sspanM[1] - request.fin.sspanM[0]},
                                               {"tip_offset_m", request.fin.xleM[1] - request.fin.xleM[0]},
                                               {"thickness_m", (request.fin.zupperOverChord[0] + request.fin.zlowerOverChord[0]) * request.fin.chordM[0]},
                                               {"axial_location_m", request.fin.xleM[0]}}}}},
            {"reference", {{"area_m2", request.reference.areaM2},
                           {"length_m", request.reference.lengthM}, {"xcg_m", request.reference.xcgM}}},
        };
    } catch (const Unsupported& error) {
        return {{"provider", "datcom_equivalent"}, {"validity", "unsupported"},
                {"diagnostic", {{"code", error.code()}, {"message", error.what()}}}};
    } catch (const std::exception& error) {
        return {{"provider", "datcom_equivalent"}, {"validity", "unsupported"},
                {"diagnostic", {{"code", "VALIDATION_FAILED"}, {"message", error.what()}}}};
    }
}

inline json classify(const json& input) { return RoketSiM::DatcomIntegration::classify(input); }
inline json analyze(const json& input) { return RoketSiM::DatcomIntegration::execute(input); }

}  // namespace datcom_equivalent::roketsim_adapter
