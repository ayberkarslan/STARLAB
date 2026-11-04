#pragma once

#include "aero/extensions/AeroExtension.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace RoketSiM::AeroExtensions {

using AeroExtensionSwitches = std::map<std::string, bool>;

enum class AeroExtensionFailurePolicy {
    SafeFallback,
    Strict,
};

struct AeroExtensionSelection {
    AeroExtensionMode mode = AeroExtensionMode::Disabled;
    std::string model;
    std::map<std::string, double> parameters;
    AeroExtensionApplicationMode applicationMode =
        AeroExtensionApplicationMode::Delta;
    bool allowComponentReplacement = false;
};

struct AeroExtensionConfiguration {
    AeroExtensionFailurePolicy failurePolicy =
        AeroExtensionFailurePolicy::SafeFallback;
    std::map<std::string, AeroExtensionSelection> extensions;
};

enum class AeroExtensionStatus {
    Disabled,
    DiagnosticOnly,
    Applied,
    Unavailable,
    Rejected,
    Failed,
};

const char* aeroExtensionStatusName(AeroExtensionStatus status) noexcept;
const char* aeroExtensionModeName(AeroExtensionMode mode) noexcept;
const char* aeroExtensionValidityName(AeroExtensionValidity validity) noexcept;
const char* aeroExtensionLifecycleName(AeroExtensionLifecycle lifecycle) noexcept;
const char* aeroExtensionApplicationModeName(
    AeroExtensionApplicationMode mode) noexcept;
const char* aeroExtensionFailurePolicyName(
    AeroExtensionFailurePolicy policy) noexcept;

struct AeroExtensionRecord {
    AeroExtensionMetadata metadata;
    bool enabled = false;
    AeroExtensionMode mode = AeroExtensionMode::Disabled;
    std::string configuredModel;
    std::map<std::string, double> configuredParameters;
    AeroExtensionApplicationMode requestedApplicationMode =
        AeroExtensionApplicationMode::Delta;
    AeroExtensionStatus status = AeroExtensionStatus::Disabled;
    AeroExtensionValidity validity = AeroExtensionValidity::NotEvaluated;
    AeroExtensionDelta delta;
    std::vector<std::string> diagnostics;
    std::map<std::string, double> diagnosticValues;
};

struct AeroExtensionEvaluation {
    BaselineAeroResult baseline;
    BaselineAeroResult finalResult;
    std::vector<AeroExtensionRecord> records;
    std::vector<std::string> unknownEnabledExtensionIds;
    AeroExtensionFailurePolicy failurePolicy =
        AeroExtensionFailurePolicy::SafeFallback;
    bool augmented = false;
};

class AeroExtensionPipeline {
public:
    explicit AeroExtensionPipeline(
        std::vector<std::shared_ptr<const AeroExtension>> extensions = {});

    const std::vector<std::shared_ptr<const AeroExtension>>& extensions() const noexcept;

    AeroExtensionEvaluation evaluate(
        const RocketGeometry& geometry,
        const AeroState& state,
        const BaselineAeroResult& baseline,
        const AeroExtensionContext& context,
        const AeroExtensionConfiguration& configuration
    ) const;

    // Compatibility overload for existing boolean callers. True maps to
    // ACTIVE and false maps to DISABLED under SAFE_FALLBACK.
    AeroExtensionEvaluation evaluate(
        const RocketGeometry& geometry,
        const AeroState& state,
        const BaselineAeroResult& baseline,
        const AeroExtensionContext& context,
        const AeroExtensionSwitches& switches = {}
    ) const;

private:
    std::vector<std::shared_ptr<const AeroExtension>> extensions_;
};

} // namespace RoketSiM::AeroExtensions
