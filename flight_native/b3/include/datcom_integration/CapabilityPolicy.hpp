#pragma once
#include <array>
#include <string>
#include <vector>
#include "datcom_integration/AdmissionDecision.hpp"
namespace RoketSiM::DatcomIntegration {
enum class ProductDisposition { Available, AvailableWithWarning, IntentionallyDisabled, BlockedByNativeLimitation, NotImplemented, NotRepresentable };
struct CapabilityPolicyEntry { CapabilityId id; ProductDisposition disposition; const char* evidenceId; const char* limitationId; };
class CapabilityPolicy { public: static constexpr const char* kPolicyVersion="datcom-capability-policy-1.5.0"; static const std::array<CapabilityPolicyEntry,42>& catalogue(); static const CapabilityPolicyEntry& entry(CapabilityId id); static AdmissionDecision evaluate(const std::vector<CapabilityId>& requested,const std::vector<Diagnostic>& classifierDiagnostics); };
std::string toString(ProductDisposition value);
}
