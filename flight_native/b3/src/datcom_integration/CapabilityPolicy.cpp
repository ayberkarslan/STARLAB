#include "datcom_integration/CapabilityPolicy.hpp"
#include <algorithm>
#include <stdexcept>
namespace RoketSiM::DatcomIntegration { namespace { using C=CapabilityId; using P=ProductDisposition; const std::array<CapabilityPolicyEntry,42> k{{
{C::BodySharpCone,P::Available,"final_sharp_cone_single_finset_static_freeze",""},{C::BodyTangentOgive,P::Available,"tangent_ogive_capability_closure",""},{C::BodyTangentOgiveSupersonic,P::NotImplemented,"tangent_ogive_capability_closure","BUG-023"},{C::BodyParabolic,P::NotImplemented,"nose_family_geometry_fidelity",""},{C::BodyPower,P::NotImplemented,"nose_family_geometry_fidelity","BUG-022"},{C::BodyHaack,P::NotImplemented,"nose_family_geometry_fidelity",""},{C::BodyVonKarman,P::NotImplemented,"nose_family_geometry_fidelity",""},{C::BodyEllipsoid,P::NotImplemented,"nose_family_geometry_fidelity",""},{C::BodyUnknownNoseFamily,P::NotRepresentable,"nose_family_geometry_fidelity",""},{C::BodyEllipticalCrossSection,P::NotRepresentable,"targeted_body_nose_geometries_re",""},{C::BodyBluntSphericalCap,P::NotRepresentable,"blunt_truncation_body_source_audit",""},{C::BodyBluntPartialRadius,P::NotRepresentable,"blunt_truncation_body_source_audit","BUG-014"},{C::BodyTruncatedNose,P::NotRepresentable,"blunt_truncation_body_source_audit","BUG-008a"},{C::BodyBoattailConical,P::NotImplemented,"body_axial_afterbody_source_audit",""},{C::BodyBoattailOgival,P::BlockedByNativeLimitation,"ogive_afterbody_failure_audit","BUG-004"},{C::BodyFlareConical,P::NotImplemented,"body_axial_afterbody_source_audit",""},{C::BodyFlareOgival,P::BlockedByNativeLimitation,"targeted_flare_ogive_combination_re","BUG-004"},{C::BodyOption2Coordinates,P::NotRepresentable,"axibod_option2_source_audit","BUG-012"},{C::FinsetSingle,P::Available,"final_sharp_cone_single_finset_static_freeze",""},{C::FinsetMultiple,P::NotImplemented,"targeted_multi_finset_coordinator_re",""},{C::FinNpanelVariants,P::Available,"fin_cao_integration_validation",""},{C::FinCrossSection,P::Available,"targeted_fin_type0_pressure_re",""},{C::FinAxial,P::Available,"targeted_fin_axial_re",""},{C::FinNormalForce,P::Available,"targeted_fin_normal_force_re",""},{C::FinMomentCp,P::Available,"targeted_fin_moment_cp_re",""},{C::BodyFinInterference,P::Available,"targeted_body_fin_interference_re","BUG-005"},{C::FinFinInterference,P::NotImplemented,"targeted_multi_finset_coordinator_re",""},{C::ControlDeflct,P::NotImplemented,"targeted_fin_cant_re",""},{C::FinOrientation,P::Available,"targeted_beta_roll_orientation_re",""},{C::BetaOrientation,P::NotImplemented,"targeted_beta_roll_orientation_re","BUG-001"},{C::StaticDerivatives,P::NotImplemented,"targeted_static_derivatives_re","BUG-002"},{C::DynamicDerivatives,P::NotImplemented,"targeted_dynamic_derivatives_re","BUG-006"},{C::MagnusSpinDerivatives,P::BlockedByNativeLimitation,"targeted_dynamic_derivatives_re","BUG-003"},{C::TrimPitch,P::NotImplemented,"targeted_trim_re","BUG-007"},{C::BaseDrag,P::NotImplemented,"body_axial_afterbody_source_audit",""},{C::BasePlumeChapmanKorst,P::NotRepresentable,"body_axial_afterbody_source_audit","BUG-008b"},{C::Protub,P::NotImplemented,"targeted_coordinate_body_re","BUG-016"},{C::Inlet,P::BlockedByNativeLimitation,"targeted_inlet_re","BUG-015"},{C::HypersBodyOnly,P::NotImplemented,"targeted_high_mach_regime_branches_re",""},{C::HyperFin,P::BlockedByNativeLimitation,"targeted_high_mach_regime_branches_re","BUG-005"},{C::ArcType2Fin,P::NotRepresentable,"targeted_arc_finset_type2_reeval_re",""},{C::SecantOgiveNamed,P::IntentionallyDisabled,"nose_family_geometry_fidelity",""}}};
Diagnostic d(const CapabilityPolicyEntry& item) {
    Diagnostic result;
    result.capabilityId = item.id;
    result.evidenceId = item.evidenceId && *item.evidenceId ? std::optional<std::string>(item.evidenceId) : std::nullopt;
    result.limitationId = item.limitationId && *item.limitationId ? std::optional<std::string>(item.limitationId) : std::nullopt;
    switch (item.disposition) {
    case P::Available:
        result.id = "DATCOM_CAPABILITY_AVAILABLE"; result.severity = DiagnosticSeverity::Info;
        result.reason = DiagnosticReason::ValidityLimited;
        result.message = "Capability is available in the compatibility operation."; break;
    case P::AvailableWithWarning:
        result.id = "DATCOM_CAPABILITY_WARNING"; result.severity = DiagnosticSeverity::Warning;
        result.reason = DiagnosticReason::ValidityLimited;
        result.message = "Capability is available with a documented validity limitation."; break;
    case P::IntentionallyDisabled:
        result.id = "DATCOM_CAPABILITY_INTENTIONALLY_DISABLED"; result.severity = DiagnosticSeverity::Error;
        result.reason = DiagnosticReason::IntentionallyDisabled;
        result.message = "Capability is intentionally disabled by product policy."; break;
    case P::BlockedByNativeLimitation:
        result.id = "DATCOM_CAPABILITY_NATIVE_BLOCKED"; result.severity = DiagnosticSeverity::Error;
        result.reason = DiagnosticReason::KnownNativeLimitation;
        result.message = "Capability is blocked by a known native limitation."; break;
    case P::NotImplemented:
        result.id = "DATCOM_CAPABILITY_NOT_IMPLEMENTED"; result.severity = DiagnosticSeverity::Error;
        result.reason = DiagnosticReason::NotImplemented;
        result.message = "Capability is classified but has no production numerical operation."; break;
    case P::NotRepresentable:
        result.id = "DATCOM_CAPABILITY_NOT_REPRESENTABLE"; result.severity = DiagnosticSeverity::Error;
        result.reason = DiagnosticReason::NotRepresentable;
        result.message = "Capability cannot be represented by the current canonical input contract."; break;
    }
    return result;
}
}

const std::array<CapabilityPolicyEntry,42>& CapabilityPolicy::catalogue(){return k;} const CapabilityPolicyEntry& CapabilityPolicy::entry(CapabilityId id){auto it=std::find_if(k.begin(),k.end(),[=](auto&e){return e.id==id;});if(it==k.end())throw std::logic_error("capability missing");return*it;} AdmissionDecision CapabilityPolicy::evaluate(const std::vector<CapabilityId>& r,const std::vector<Diagnostic>& cd){AdmissionDecision o;o.requestedCapabilities=r;o.diagnostics=cd;o.policyVersion=kPolicyVersion;for(auto id:r){auto&e=entry(id);if(e.disposition==P::Available||e.disposition==P::AvailableWithWarning){o.acceptedCapabilities.push_back(id);if(e.disposition==P::AvailableWithWarning)o.diagnostics.push_back(d(e));}else{o.rejectedCapabilities.push_back(id);o.diagnostics.push_back(d(e));}}bool er=std::any_of(o.diagnostics.begin(),o.diagnostics.end(),[](auto&x){return x.severity==DiagnosticSeverity::Error;}),w=std::any_of(o.diagnostics.begin(),o.diagnostics.end(),[](auto&x){return x.severity==DiagnosticSeverity::Warning;});o.disposition=er?AdmissionDisposition::Reject:w?AdmissionDisposition::AllowWithWarning:AdmissionDisposition::Allow;return o;}
std::string toString(ProductDisposition x){static const char* n[]={"AVAILABLE","AVAILABLE_WITH_WARNING","INTENTIONALLY_DISABLED","BLOCKED_BY_NATIVE_LIMITATION","NOT_IMPLEMENTED","NOT_REPRESENTABLE"};return n[(int)x];}std::string toString(DiagnosticSeverity x){return x==DiagnosticSeverity::Info?"info":x==DiagnosticSeverity::Warning?"warning":"error";}std::string toString(DiagnosticReason x){static const char*n[]={"INVALID_REQUEST","NOT_REPRESENTABLE","NOT_IMPLEMENTED","KNOWN_NATIVE_LIMITATION","INTENTIONALLY_DISABLED","VALIDITY_LIMITED","EXPLICIT_EXCLUSION","NUMERICAL_INVARIANT"};return n[(int)x];}std::string toString(AdmissionDisposition x){return x==AdmissionDisposition::Allow?"ALLOW":x==AdmissionDisposition::AllowWithWarning?"ALLOW_WITH_WARNING":"REJECT";}}
