#pragma once

#include <string>
#include <vector>
#include "datcom_integration/CapabilityId.hpp"
#include "datcom_integration/Diagnostic.hpp"
namespace RoketSiM::DatcomIntegration {
enum class AdmissionDisposition { Allow, AllowWithWarning, Reject };
struct AdmissionDecision { AdmissionDisposition disposition = AdmissionDisposition::Reject; std::vector<CapabilityId> requestedCapabilities; std::vector<CapabilityId> acceptedCapabilities; std::vector<CapabilityId> rejectedCapabilities; std::vector<Diagnostic> diagnostics; std::string policyVersion; };
std::string toString(AdmissionDisposition value);
}
