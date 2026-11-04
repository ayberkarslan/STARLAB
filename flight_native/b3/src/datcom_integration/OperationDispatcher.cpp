#include "datcom_integration/Integration.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "datcom_equivalent/RawStaticSolver.hpp"
#include "datcom_integration/CapabilityPolicy.hpp"
#include "datcom_integration/Provenance.hpp"
#include "datcom_integration/ResultContract.hpp"

namespace RoketSiM::DatcomIntegration {
namespace {

using json = nlohmann::json;

json idsToJson(const std::vector<CapabilityId>& values) {
    json result = json::array();
    for (const auto id : values) result.push_back(toString(id));
    return result;
}

json provenance(const ClassifiedRequest& classified, const AdmissionDecision& decision,
                const char* operation) {
    json diagnosticIds = json::array(), warningIds = json::array(), limitationIds = json::array();
    for (const auto& item : decision.diagnostics) {
        diagnosticIds.push_back(item.id);
        if (item.severity == DiagnosticSeverity::Warning) warningIds.push_back(item.id);
        if (item.limitationId
            && std::find(limitationIds.begin(), limitationIds.end(), *item.limitationId) == limitationIds.end()) {
            limitationIds.push_back(*item.limitationId);
        }
    }
    return {
        {"provider_id", ProvenanceContract::kProviderId},
        {"provider_version", ProvenanceContract::kProviderVersion},
        {"mapper_version", ProvenanceContract::kMapperVersion},
        {"policy_version", decision.policyVersion},
        {"engine_version", ProvenanceContract::kEngineVersion},
        {"result_contract_version", ProvenanceContract::kResultContractVersion},
        {"recovery_provenance_id", ProvenanceContract::kRecoveryProvenanceId},
        {"requested_capability_ids", idsToJson(classified.requestedCapabilities)},
        {"admission_disposition", toString(decision.disposition)},
        {"actual_operation", operation},
        {"diagnostic_ids", diagnosticIds}, {"warning_ids", warningIds},
        {"limitation_ids", limitationIds},
        {"frame_id", classified.mapped.reference.frameId},
        {"reference", {{"area_m2", classified.mapped.reference.areaM2},
                       {"length_m", classified.mapped.reference.lengthM},
                       {"diameter_m", classified.mapped.reference.diameterM},
                       {"xcg_m", classified.mapped.reference.xcgM}}},
    };
}

json catalogueJson() {
    json result = json::array();
    for (const auto& item : CapabilityPolicy::catalogue()) result.push_back({
        {"capability_id", toString(item.id)}, {"product_disposition", toString(item.disposition)},
        {"evidence_id", item.evidenceId && *item.evidenceId ? json(item.evidenceId) : json(nullptr)},
        {"limitation_id", item.limitationId && *item.limitationId ? json(item.limitationId) : json(nullptr)}});
    return result;
}

json classificationResponse(const ClassifiedRequest& classified, const AdmissionDecision& decision) {
    json diagnostics = json::array();
    for (const auto& item : decision.diagnostics) diagnostics.push_back(diagnosticToJson(item));
    return {
        {"provider", ProvenanceContract::kProviderId}, {"display_name", "DATCOM Equivalent"},
        {"classification_only", true}, {"mapped_request", mappedRequestToJson(classified.mapped)},
        {"requested_capabilities", idsToJson(classified.requestedCapabilities)},
        {"observed_capabilities", idsToJson(classified.observedCapabilities)},
        {"admission", admissionToJson(decision)}, {"diagnostics", diagnostics},
        {"policy_version", decision.policyVersion}, {"capability_catalogue", catalogueJson()},
        {"provenance", provenance(classified, decision, "NONE")},
    };
}

AdmissionDecision admission(const ClassifiedRequest& classified) {
    auto decision = CapabilityPolicy::evaluate(classified.requestedCapabilities, classified.diagnostics);
    if (decision.disposition != AdmissionDisposition::Reject && classified.mapped.finsets.size() == 1) {
        const auto& fin = classified.mapped.finsets.front();
        if (fin.exactPlanform && fin.exactPlanform->source == FinPlanformSource::FreeformSourceOutline) {
            Diagnostic diagnostic{"DATCOM_FREEFORM_FINSET_ADMITTED", DiagnosticSeverity::Info,
                CapabilityId::FinsetSingle,
                "Exact source-derived two-station Freeform Fin Set admitted to the existing single-FINSET operation; no higher validation claim.",
                DiagnosticReason::ValidityLimited};
            diagnostic.sourceComponentId = fin.sourceComponentId;
            diagnostic.sectionId = toString(fin.sectionFamily);
            decision.diagnostics.push_back(std::move(diagnostic));
        }
    }
    return decision;
}

std::string legacyCode(const ClassifiedRequest& classified, const AdmissionDecision& decision) {
    const auto& mapped = classified.mapped;
    if (mapped.body.family != BodyFamily::SharpCone
        && mapped.body.family != BodyFamily::TangentOgive) return "UNSUPPORTED_NOSE_FAMILY";
    if (mapped.finsets.size() > 1) return "MULTIPLE_FINSETS";
    if (!mapped.finsets.empty() && mapped.finsets.front().panelCount != 3
        && mapped.finsets.front().panelCount != 4 && mapped.finsets.front().panelCount != 6) return "UNSUPPORTED_PANEL_COUNT";
    if (mapped.condition.mach < .2 || mapped.condition.mach > 5.) return "MACH_OUT_OF_RANGE";
    if (mapped.condition.alphaDeg < -10. || mapped.condition.alphaDeg > 10.) return "ALPHA_OUT_OF_RANGE";
    if (mapped.condition.betaDeg != 0.) return "NONZERO_BETA";
    if (mapped.condition.finCantDeg != 0.) return "NONZERO_CANT";
    if (mapped.condition.controlDeflectionDeg != 0.) return "NONZERO_CONTROL_DEFLECTION";
    const auto error = std::find_if(decision.diagnostics.begin(), decision.diagnostics.end(),
        [](const auto& value) { return value.severity == DiagnosticSeverity::Error; });
    return error == decision.diagnostics.end() ? "VALIDATION_FAILED" : error->id;
}

json channel(double value, bool warning) {
    return {{"status", warning ? "WARNING_LIMITED" : "AVAILABLE"}, {"value", value},
            {"diagnostics", json::array()}};
}

}  // namespace

datcom_equivalent::DatcomEquivalentRequest compatibilityRequest(const ClassifiedRequest& classified) {
    const auto& mapped = classified.mapped;
    if ((mapped.body.family != BodyFamily::SharpCone
         && mapped.body.family != BodyFamily::TangentOgive) || mapped.bodySections.size() != 1
        || mapped.bodySections.front().family != "CONSTANT_CYLINDER" || mapped.finsets.size() != 1
        || mapped.condition.analysisMode != AnalysisMode::StaticLongitudinal
        || (mapped.body.family == BodyFamily::TangentOgive && mapped.condition.mach > 1.2)) {
        throw std::invalid_argument("request does not match an implemented static-longitudinal single-FINSET operation");
    }
    const auto& body = mapped.bodySections.front();
    const auto convertedFinsets = convertMappedFinsets(mapped);
    if (convertedFinsets.size() != 1 || !convertedFinsets.front().hasExactProviderGeometry) {
        throw std::invalid_argument("request does not contain one exact-supported FINSET provider geometry");
    }
    const auto& fin = convertedFinsets.front();
    const double diameter = mapped.body.diameterM > 0.0 ? mapped.body.diameterM : body.foreDiameterM;
    datcom_equivalent::DatcomEquivalentRequest request;
    request.body = {mapped.body.lengthM, diameter, body.lengthM, mapped.body.originM};
    request.body.noseFamily = mapped.body.family == BodyFamily::TangentOgive
        ? datcom_equivalent::BodyNoseFamily::TangentOgive
        : datcom_equivalent::BodyNoseFamily::SharpCone;
    request.fin = fin.geometry;
    request.flight = {mapped.condition.mach, mapped.condition.reynoldsPerM,
                      mapped.condition.boundaryLayer, mapped.condition.roughnessM};
    request.reference = {mapped.reference.areaM2, mapped.reference.lengthM, mapped.reference.xcgM};
    request.alphaDeg = mapped.condition.alphaDeg;
    request.betaDeg = mapped.condition.betaDeg;
    request.finCantDeg = mapped.condition.finCantDeg;
    request.controlDeflectionDeg = mapped.condition.controlDeflectionDeg;
    return request;
}

nlohmann::json classify(const nlohmann::json& input) {
    auto classified = classifyRequest(mapGeometryRequest(input));
    const auto decision = admission(classified);
    return classificationResponse(classified, decision);
}

nlohmann::json execute(const nlohmann::json& input) {
    auto classified = classifyRequest(mapGeometryRequest(input));
    const auto decision = admission(classified);
    if (decision.disposition == AdmissionDisposition::Reject) {
        const auto firstError = std::find_if(decision.diagnostics.begin(), decision.diagnostics.end(),
            [](const auto& value) { return value.severity == DiagnosticSeverity::Error; });
        const std::string message = firstError == decision.diagnostics.end()
            ? "Request rejected by DATCOM capability policy." : firstError->message;
        json response = classificationResponse(classified, decision);
        response["classification_only"] = false;
        response["validity"] = "unsupported";
        response["diagnostic"] = {{"code", legacyCode(classified, decision)}, {"message", message}};
        response["typed_result"] = unavailableTypedResults(classified.mapped.reference.frameId);
        response["provenance"] = provenance(classified, decision, "REJECTED");
        return response;
    }

    try {
        const auto request = compatibilityRequest(classified);
        const auto result = datcom_equivalent::evaluateRawStaticLongitudinal(request);
        const char* operation = request.body.noseFamily == datcom_equivalent::BodyNoseFamily::TangentOgive
            ? "STATIC_LONGITUDINAL_TANGENT_OGIVE_SINGLE_FINSET_SUBTRANS"
            : "STATIC_LONGITUDINAL_CONE_SINGLE_FINSET";
        const double alpha = request.alphaDeg * 3.14159265358979323846 / 180.0;
        const bool cpDefined = std::abs(result.finalCn) > 1e-8;
        const double cp = cpDefined ? request.reference.xcgM
            - result.finalCm * request.reference.lengthM / result.finalCn : 0.0;
        const double lift = result.finalCn * std::cos(alpha) - result.finalCa * std::sin(alpha);
        const double drag = result.finalCa * std::cos(alpha) + result.finalCn * std::sin(alpha);
        json panels = json::array();
        for (std::size_t index = 0; index < result.panelCn.size(); ++index) panels.push_back({
            {"panel", static_cast<int>(index + 1)},
            {"azimuth_deg", index < request.fin.panelAzimuthsDeg.size() ? request.fin.panelAzimuthsDeg[index] : 0.0},
            {"equivalent_alpha_deg", index < result.equivalentPanelAnglesDeg.size() ? result.equivalentPanelAnglesDeg[index] : 0.0},
            {"cn", result.panelCn[index]}, {"projected_contribution", result.panelCn[index]}});
        const json breakdown = {
            {"body", {{"ca_total", result.bodyCa}, {"cn_total", result.bodyCn}, {"cm_total", result.bodyCm}}},
            {"fin", {{"ca", result.isolatedFinCa}, {"cna_per_rad", result.isolatedFinCna},
                     {"cn", result.isolatedFinCn}, {"cm", result.isolatedFinCm},
                     {"cp_m", result.isolatedFinCp ? json(*result.isolatedFinCp) : json(nullptr)}}},
            {"body_fin", {{"kw_base", result.kwBase}, {"kw_alpha", result.kw}, {"kb", result.kb},
                          {"kkw", result.kkw}, {"kkb", result.kkb}, {"bdwk", result.bdwk},
                          {"finset_in_body_cn", result.finInBodyCn}, {"carryover_cn", result.carryoverCn},
                          {"carryover_cm", result.carryoverCm}}}, {"panels", panels}};
        auto executionDecision = decision;
        if (!cpDefined) {
            executionDecision.diagnostics.push_back({"DATCOM_CP_UNAVAILABLE_NEAR_ZERO_CN",
                DiagnosticSeverity::Warning,
                request.body.noseFamily == datcom_equivalent::BodyNoseFamily::TangentOgive
                    ? std::optional<CapabilityId>(CapabilityId::BodyTangentOgive)
                    : std::optional<CapabilityId>(CapabilityId::BodySharpCone),
                "Center of pressure is unavailable because abs(CN) is at or below 1e-8.",
                DiagnosticReason::NumericalInvariant,
                std::string("tangent_ogive_capability_closure"), std::nullopt});
            if (executionDecision.disposition == AdmissionDisposition::Allow)
                executionDecision.disposition = AdmissionDisposition::AllowWithWarning;
        }
        json typed = unavailableTypedResults(classified.mapped.reference.frameId);
        const bool warning = decision.disposition == AdmissionDisposition::AllowWithWarning;
        typed["coefficients"]["axial_force"] = channel(result.finalCa, warning);
        typed["coefficients"]["normal_force"] = channel(result.finalCn, warning);
        typed["coefficients"]["lift"] = channel(lift, warning);
        typed["coefficients"]["drag"] = channel(drag, warning);
        typed["coefficients"]["pitch_moment"] = channel(result.finalCm, warning);
        typed["coefficients"]["center_of_pressure"] = cpDefined ? channel(cp, warning)
            : json{{"status", "UNAVAILABLE"}, {"value", nullptr},
                   {"diagnostics", json::array({"DATCOM_CP_UNAVAILABLE_NEAR_ZERO_CN"})}};
        json structuredWarnings = json::array();
        for (const auto& item : executionDecision.diagnostics) if (item.severity == DiagnosticSeverity::Warning)
            structuredWarnings.push_back(diagnosticToJson(item));
        json response = {
            {"provider", ProvenanceContract::kProviderId}, {"display_name", "DATCOM Equivalent"},
            {"validity", "supported"}, {"ca", result.finalCa}, {"cn", result.finalCn}, {"cm", result.finalCm},
            {"cp_m", cpDefined ? json(cp) : json(nullptr)}, {"cl", lift}, {"cd", drag},
            {"method", result.activeBodyMethod}, {"body_fallback", result.bodyFallback},
            {"warnings", result.warnings}, {"structured_warnings", structuredWarnings},
            {"diagnostics", admissionToJson(executionDecision)["diagnostics"]}, {"admission", admissionToJson(executionDecision)},
            {"component_breakdown", breakdown}, {"typed_result", typed},
            {"mapped_geometry", {{"body", {{"family", request.body.noseFamily == datcom_equivalent::BodyNoseFamily::TangentOgive ? "Tangent Ogive + Cylinder" : "Sharp Cone + Cylinder"}, {"diameter_m", request.body.diameterM},
                                               {"nose_length_m", request.body.noseLengthM},
                                               {"cylinder_length_m", request.body.centerLengthM}}},
                                 {"finset", {{"count", request.fin.npanel}, {"root_chord_m", request.fin.chordM[0]},
                                               {"tip_chord_m", request.fin.chordM[1]},
                                               {"span_m", request.fin.sspanM[1] - request.fin.sspanM[0]},
                                               {"tip_offset_m", request.fin.xleM[1] - request.fin.xleM[0]},
                                               {"thickness_m", classified.mapped.finsets.front().thicknessM},
                                               {"axial_location_m", request.fin.xleM[0]}}}}},
            {"reference", {{"area_m2", request.reference.areaM2}, {"length_m", request.reference.lengthM},
                           {"xcg_m", request.reference.xcgM}}},
            {"provenance", provenance(classified, executionDecision, operation)},
        };
        // Admitted outlines retain the lossless mapping in the result as well
        // as classification; executed geometry alone cannot identify the source.
        if (classified.mapped.finsets.front().exactPlanform->source == FinPlanformSource::FreeformSourceOutline)
            response["mapped_request"] = mappedRequestToJson(classified.mapped);
        for (const auto& warningText : result.warnings) response["structured_warnings"].push_back({
            {"id", "DATCOM_ENGINE_WARNING"}, {"severity", "warning"}, {"capability_id", nullptr},
            {"message", warningText}, {"reason", "NUMERICAL_INVARIANT"}, {"evidence_id", nullptr},
            {"limitation_id", nullptr}});
        return response;
    } catch (const std::exception& error) {
        Diagnostic diagnostic{"DATCOM_NUMERICAL_INVARIANT_REJECTED", DiagnosticSeverity::Error, std::nullopt,
            error.what(), DiagnosticReason::NumericalInvariant};
        auto failedDecision = decision;
        failedDecision.disposition = AdmissionDisposition::Reject;
        failedDecision.diagnostics.push_back(diagnostic);
        json response = classificationResponse(classified, failedDecision);
        response["classification_only"] = false;
        response["validity"] = "unsupported";
        response["diagnostic"] = {{"code", "VALIDATION_FAILED"}, {"message", error.what()}};
        response["typed_result"] = unavailableTypedResults(classified.mapped.reference.frameId);
        response["provenance"] = provenance(classified, failedDecision, "REJECTED_BY_ENGINE_INVARIANT");
        return response;
    }
}

}  // namespace RoketSiM::DatcomIntegration
