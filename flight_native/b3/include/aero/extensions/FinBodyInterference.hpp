#pragma once

#include "aero/extensions/AeroExtension.hpp"

#include <string>

namespace RoketSiM::AeroExtensions {

enum class FinCoefficientStage {
    RawSingleFin,
    FinCountCorrected,
    BodyInterferenceApplied,
    Unknown,
};

// The canonical fin result is deliberately passed in as data.  A later
// NACA/DATCOM port may compare isolated and installed fin states without
// reaching back into, or replacing, the canonical fin equations.
struct FinBodyInterferenceInput {
    std::string finSetId;
    int finCount = 0;
    double bodyRadiusM = 0.0;
    double finSpanM = 0.0;
    double finPlanformAreaM2 = 0.0;
    double canonicalFinAloneCnaPerRad = 0.0;
    double canonicalFinInstalledCnaPerRad = 0.0;
    FinCoefficientStage coefficientStage = FinCoefficientStage::Unknown;
};

struct FinBodyInterferenceDecomposition {
    double finInBodyMinusFinAloneCnaPerRad = 0.0;
    double bodyDueToFinCnaPerRad = 0.0;

    double totalDeltaCnaPerRad() const noexcept {
        return finInBodyMinusFinAloneCnaPerRad + bodyDueToFinCnaPerRad;
    }
};

struct FinBodyInterferenceResult {
    FinBodyInterferenceDecomposition decomposition;
    AeroExtensionValidity validity = AeroExtensionValidity::Valid;
    std::vector<std::string> diagnostics;
};

inline void recordFinBodyInterferenceDiagnostics(
    AeroExtensionResult& result,
    const FinBodyInterferenceDecomposition& decomposition
) {
    result.diagnosticValues["fin_in_body_minus_fin_alone_cna_per_rad"] =
        decomposition.finInBodyMinusFinAloneCnaPerRad;
    result.diagnosticValues["body_due_to_fin_cna_per_rad"] =
        decomposition.bodyDueToFinCnaPerRad;
    result.diagnosticValues["total_fin_body_delta_cna_per_rad"] =
        decomposition.totalDeltaCnaPerRad();
}

class FinBodyInterferenceModel {
public:
    virtual ~FinBodyInterferenceModel() = default;
    virtual FinBodyInterferenceResult evaluate(
        const FinBodyInterferenceInput& input,
        const AeroState& state
    ) const = 0;
};

} // namespace RoketSiM::AeroExtensions
