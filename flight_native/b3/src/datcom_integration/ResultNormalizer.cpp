#include "datcom_integration/ResultContract.hpp"
#include "datcom_integration/CapabilityPolicy.hpp"

#include "datcom_integration/Provenance.hpp"

namespace RoketSiM::DatcomIntegration {

std::string toString(OutputStatus value) {
    switch (value) {
    case OutputStatus::Available: return "AVAILABLE";
    case OutputStatus::WarningLimited: return "WARNING_LIMITED";
    case OutputStatus::NotRequested: return "NOT_REQUESTED";
    case OutputStatus::Unsupported: return "UNSUPPORTED";
    case OutputStatus::Invalid: return "INVALID";
    case OutputStatus::NotConverged: return "NOT_CONVERGED";
    }
    return "INVALID";
}

std::string toString(TrimStatus value) {
    switch (value) {
    case TrimStatus::Converged: return "CONVERGED";
    case TrimStatus::NotConverged: return "NOT_CONVERGED";
    case TrimStatus::Unavailable: return "UNAVAILABLE";
    case TrimStatus::Rejected: return "REJECTED";
    }
    return "UNAVAILABLE";
}

nlohmann::json diagnosticToJson(const Diagnostic& value) {
    return {
        {"source_component_id", value.sourceComponentId ? nlohmann::json(*value.sourceComponentId) : nlohmann::json(nullptr)},
        {"section_id", value.sectionId ? nlohmann::json(*value.sectionId) : nlohmann::json(nullptr)},
        {"policy_version", CapabilityPolicy::kPolicyVersion},
        {"id", value.id}, {"severity", toString(value.severity)},
        {"blocks_action", value.severity == DiagnosticSeverity::Error},
        {"capability_id", value.capabilityId ? nlohmann::json(toString(*value.capabilityId)) : nlohmann::json(nullptr)},
        {"message", value.message}, {"reason", toString(value.reason)},
        {"evidence_id", value.evidenceId ? nlohmann::json(*value.evidenceId) : nlohmann::json(nullptr)},
        {"limitation_id", value.limitationId ? nlohmann::json(*value.limitationId) : nlohmann::json(nullptr)},
    };
}

nlohmann::json admissionToJson(const AdmissionDecision& value) {
    nlohmann::json requested = nlohmann::json::array(), accepted = nlohmann::json::array();
    nlohmann::json rejected = nlohmann::json::array(), diagnostics = nlohmann::json::array();
    nlohmann::json warnings = nlohmann::json::array();
    for (const auto id : value.requestedCapabilities) requested.push_back(toString(id));
    for (const auto id : value.acceptedCapabilities) accepted.push_back(toString(id));
    for (const auto id : value.rejectedCapabilities) rejected.push_back(toString(id));
    for (const auto& diagnostic : value.diagnostics) {
        diagnostics.push_back(diagnosticToJson(diagnostic));
        if (diagnostic.severity == DiagnosticSeverity::Warning) warnings.push_back(diagnosticToJson(diagnostic));
    }
    return {{"disposition", toString(value.disposition)}, {"requested_capabilities", requested},
            {"accepted_capabilities", accepted}, {"rejected_capabilities", rejected},
            {"diagnostics", diagnostics}, {"warnings", warnings}, {"policy_version", value.policyVersion}};
}

nlohmann::json unavailableTypedResults(const std::string& frameId) {
    const auto unavailable = [] { return nlohmann::json{{"status", "NOT_REQUESTED"}, {"value", nullptr}, {"diagnostics", nlohmann::json::array()}}; };
    return {
        {"contract_version", ProvenanceContract::kResultContractVersion}, {"frame_id", frameId},
        {"coefficients", {
            {"axial_force", unavailable()}, {"normal_force", unavailable()}, {"side_force", unavailable()},
            {"lift", unavailable()}, {"drag", unavailable()}, {"roll_moment", unavailable()},
            {"pitch_moment", unavailable()}, {"yaw_moment", unavailable()}, {"center_of_pressure", unavailable()}}},
        {"derivatives", nlohmann::json::array()},
        {"trim", {{"status", "UNAVAILABLE"}, {"control_value_deg", nullptr}, {"residual", nullptr},
                  {"diagnostics", nlohmann::json::array()}}},
    };
}

}  // namespace RoketSiM::DatcomIntegration
