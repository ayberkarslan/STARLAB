#pragma once
#include <optional>
#include <string>
#include "datcom_integration/CapabilityId.hpp"
namespace RoketSiM::DatcomIntegration {
enum class DiagnosticSeverity { Info, Warning, Error }; enum class DiagnosticReason { InvalidRequest, NotRepresentable, NotImplemented, KnownNativeLimitation, IntentionallyDisabled, ValidityLimited, ExplicitExclusion, NumericalInvariant };
struct Diagnostic { std::string id; DiagnosticSeverity severity=DiagnosticSeverity::Error; std::optional<CapabilityId> capabilityId; std::string message; DiagnosticReason reason=DiagnosticReason::InvalidRequest; std::optional<std::string> evidenceId; std::optional<std::string> limitationId; std::optional<std::string> sourceComponentId; std::optional<std::string> sectionId; };
std::string toString(DiagnosticSeverity value); std::string toString(DiagnosticReason value);
}
