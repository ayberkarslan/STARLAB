#pragma once
#include <optional>
#include <string>
#include <vector>
#include "datcom_integration/AdmissionDecision.hpp"
#include "nlohmann/json.hpp"
namespace RoketSiM::DatcomIntegration { enum class OutputStatus { Available, WarningLimited, NotRequested, Unsupported, Invalid, NotConverged }; enum class TrimStatus { Converged, NotConverged, Unavailable, Rejected }; struct TypedChannel { OutputStatus status=OutputStatus::NotRequested; std::optional<double> value; std::vector<Diagnostic> diagnostics; }; struct DerivativeResult { std::string identifier,independentVariable,units,frameId; TypedChannel output; }; struct TrimResult { TrimStatus status=TrimStatus::Unavailable; std::optional<double> controlValueDeg,residual; std::vector<Diagnostic> diagnostics; }; std::string toString(OutputStatus value); std::string toString(TrimStatus value); nlohmann::json diagnosticToJson(const Diagnostic& value); nlohmann::json admissionToJson(const AdmissionDecision& value); nlohmann::json unavailableTypedResults(const std::string& frameId); }
