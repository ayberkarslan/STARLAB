#include "aero/extensions/AeroExtensionPipeline.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>

namespace RoketSiM::AeroExtensions {
namespace {

bool finite(const BodyVector& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y)
        && std::isfinite(value.z);
}

bool finite(const AeroCoefficientSet& value) noexcept {
    return std::isfinite(value.axialForce)
        && std::isfinite(value.sideForce)
        && std::isfinite(value.normalForce)
        && std::isfinite(value.rollMoment)
        && std::isfinite(value.pitchMoment)
        && std::isfinite(value.yawMoment)
        && std::isfinite(value.drag)
        && std::isfinite(value.normalForceSlopePerRad)
        && std::isfinite(value.normalForceSlopeMomentMPerRad);
}

bool finite(const AeroExtensionDelta& value) noexcept {
    return finite(value.forceN) && finite(value.momentNm)
        && finite(value.coefficients);
}

void add(BodyVector& target, const BodyVector& delta) noexcept {
    target.x += delta.x;
    target.y += delta.y;
    target.z += delta.z;
}

void add(AeroCoefficientSet& target, const AeroCoefficientSet& delta) noexcept {
    target.axialForce += delta.axialForce;
    target.sideForce += delta.sideForce;
    target.normalForce += delta.normalForce;
    target.rollMoment += delta.rollMoment;
    target.pitchMoment += delta.pitchMoment;
    target.yawMoment += delta.yawMoment;
    target.drag += delta.drag;
    target.normalForceSlopePerRad += delta.normalForceSlopePerRad;
    target.normalForceSlopeMomentMPerRad += delta.normalForceSlopeMomentMPerRad;
}

} // namespace

const char* aeroExtensionStatusName(AeroExtensionStatus status) noexcept {
    switch (status) {
    case AeroExtensionStatus::Disabled: return "DISABLED";
    case AeroExtensionStatus::DiagnosticOnly: return "DIAGNOSTIC_ONLY";
    case AeroExtensionStatus::Applied: return "APPLIED";
    case AeroExtensionStatus::Unavailable: return "UNAVAILABLE";
    case AeroExtensionStatus::Rejected: return "REJECTED";
    case AeroExtensionStatus::Failed: return "FAILED";
    }
    return "FAILED";
}

const char* aeroExtensionModeName(AeroExtensionMode mode) noexcept {
    switch (mode) {
    case AeroExtensionMode::Disabled: return "DISABLED";
    case AeroExtensionMode::DiagnosticOnly: return "DIAGNOSTIC_ONLY";
    case AeroExtensionMode::Active: return "ACTIVE";
    }
    return "DISABLED";
}

const char* aeroExtensionValidityName(AeroExtensionValidity validity) noexcept {
    switch (validity) {
    case AeroExtensionValidity::NotEvaluated: return "NOT_EVALUATED";
    case AeroExtensionValidity::Valid: return "VALID";
    case AeroExtensionValidity::MethodLimited: return "METHOD_LIMITED";
    case AeroExtensionValidity::OutOfRange: return "OUT_OF_RANGE";
    case AeroExtensionValidity::UnsupportedGeometry: return "UNSUPPORTED_GEOMETRY";
    case AeroExtensionValidity::MissingInput: return "MISSING_INPUT";
    case AeroExtensionValidity::NumericallyInvalid: return "NUMERICALLY_INVALID";
    case AeroExtensionValidity::ReferenceOnly: return "REFERENCE_ONLY";
    case AeroExtensionValidity::NotImplemented: return "NOT_IMPLEMENTED";
    case AeroExtensionValidity::InvalidInputStage: return "INVALID_INPUT_STAGE";
    case AeroExtensionValidity::DependencyUnavailable: return "DEPENDENCY_UNAVAILABLE";
    case AeroExtensionValidity::InternalError: return "INTERNAL_ERROR";
    }
    return "INTERNAL_ERROR";
}

const char* aeroExtensionLifecycleName(AeroExtensionLifecycle lifecycle) noexcept {
    switch (lifecycle) {
    case AeroExtensionLifecycle::ReferenceOnly: return "REFERENCE_ONLY";
    case AeroExtensionLifecycle::Experimental: return "EXPERIMENTAL";
    case AeroExtensionLifecycle::Validated: return "VALIDATED";
    case AeroExtensionLifecycle::Production: return "PRODUCTION";
    case AeroExtensionLifecycle::Deprecated: return "DEPRECATED";
    }
    return "REFERENCE_ONLY";
}

const char* aeroExtensionApplicationModeName(
    AeroExtensionApplicationMode mode) noexcept {
    switch (mode) {
    case AeroExtensionApplicationMode::Delta: return "DELTA";
    case AeroExtensionApplicationMode::ComponentReplacement:
        return "COMPONENT_REPLACEMENT";
    }
    return "DELTA";
}

const char* aeroExtensionFailurePolicyName(
    AeroExtensionFailurePolicy policy) noexcept {
    switch (policy) {
    case AeroExtensionFailurePolicy::SafeFallback: return "SAFE_FALLBACK";
    case AeroExtensionFailurePolicy::Strict: return "STRICT";
    }
    return "SAFE_FALLBACK";
}

AeroExtensionPipeline::AeroExtensionPipeline(
    std::vector<std::shared_ptr<const AeroExtension>> extensions)
    : extensions_(std::move(extensions)) {
    std::set<std::string> ids;
    for (const auto& extension : extensions_) {
        if (!extension) {
            throw std::invalid_argument("Aero extension registry contains a null extension.");
        }
        const AeroExtensionMetadata metadata = extension->metadata();
        if (metadata.id.empty()) {
            throw std::invalid_argument("Aero extension IDs must not be empty.");
        }
        if (!ids.insert(metadata.id).second) {
            throw std::invalid_argument("Duplicate aero extension ID: " + metadata.id);
        }
    }
}

const std::vector<std::shared_ptr<const AeroExtension>>&
AeroExtensionPipeline::extensions() const noexcept {
    return extensions_;
}

AeroExtensionEvaluation AeroExtensionPipeline::evaluate(
    const RocketGeometry& geometry,
    const AeroState& state,
    const BaselineAeroResult& baseline,
    const AeroExtensionContext& context,
    const AeroExtensionConfiguration& configuration
) const {
    AeroExtensionEvaluation evaluation;
    evaluation.baseline = baseline;
    evaluation.failurePolicy = configuration.failurePolicy;
    // This assignment is the zero-regression guard: disabled/unavailable/
    // invalid extensions never reconstruct or normalize baseline values.
    evaluation.finalResult = baseline;

    std::set<std::string> registeredIds;
    std::map<std::string, AeroExtensionMetadata> registeredMetadata;
    for (const auto& extension : extensions_) {
        const auto metadata = extension->metadata();
        registeredIds.insert(metadata.id);
        registeredMetadata.emplace(metadata.id, std::move(metadata));
    }
    evaluation.records.reserve(extensions_.size());
    for (const auto& extension : extensions_) {
        AeroExtensionRecord record;
        record.metadata = extension->metadata();
        const auto selected = configuration.extensions.find(record.metadata.id);
        if (selected != configuration.extensions.end()) {
            record.mode = selected->second.mode;
            record.configuredModel = selected->second.model;
            record.configuredParameters = selected->second.parameters;
            record.requestedApplicationMode = selected->second.applicationMode;
        }
        record.enabled = record.mode != AeroExtensionMode::Disabled;
        if (!record.enabled) {
            evaluation.records.push_back(std::move(record));
            continue;
        }

        if (!record.configuredModel.empty()
            && record.configuredModel != record.metadata.id) {
            record.status = AeroExtensionStatus::Rejected;
            record.validity = AeroExtensionValidity::InvalidInputStage;
            record.diagnostics.push_back(
                "Configured model does not match the registered extension ID: "
                + record.configuredModel);
            if (configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw std::runtime_error(
                    "Strict aero extension failure: configured model mismatch for "
                    + record.metadata.id + ".");
            }
            evaluation.records.push_back(std::move(record));
            continue;
        }
        const bool parametersFinite = std::all_of(
            record.configuredParameters.begin(), record.configuredParameters.end(),
            [](const auto& parameter) { return std::isfinite(parameter.second); });
        if (!parametersFinite) {
            record.status = AeroExtensionStatus::Rejected;
            record.validity = AeroExtensionValidity::NumericallyInvalid;
            record.diagnostics.push_back(
                "Extension configuration contains a non-finite parameter.");
            if (configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw std::runtime_error(
                    "Strict aero extension failure: non-finite parameter for "
                    + record.metadata.id + ".");
            }
            evaluation.records.push_back(std::move(record));
            continue;
        }

        std::vector<std::string> missingDependencies;
        for (const auto& dependency : record.metadata.dependencies) {
            const auto dependencyMetadata = registeredMetadata.find(dependency);
            const auto dependencySelection =
                configuration.extensions.find(dependency);
            if (dependencyMetadata == registeredMetadata.end()
                || !dependencyMetadata->second.available
                || dependencySelection == configuration.extensions.end()
                || dependencySelection->second.mode == AeroExtensionMode::Disabled) {
                missingDependencies.push_back(dependency);
            }
        }
        if (!missingDependencies.empty()) {
            record.status = AeroExtensionStatus::Unavailable;
            record.validity = AeroExtensionValidity::DependencyUnavailable;
            for (const auto& dependency : missingDependencies) {
                record.diagnostics.push_back(
                    "Required extension dependency is unavailable or disabled: "
                    + dependency);
            }
            if (configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw std::runtime_error(
                    "Strict aero extension failure: dependency unavailable for "
                    + record.metadata.id + ".");
            }
            evaluation.records.push_back(std::move(record));
            continue;
        }
        if (!record.metadata.available) {
            record.status = AeroExtensionStatus::Unavailable;
            record.validity = record.metadata.lifecycle
                    == AeroExtensionLifecycle::ReferenceOnly
                ? AeroExtensionValidity::ReferenceOnly
                : AeroExtensionValidity::NotImplemented;
            record.diagnostics.push_back(
                "Extension is catalogued but has no validated production implementation.");
            if (configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw std::runtime_error(
                    "Strict aero extension failure: " + record.metadata.id
                    + " is unavailable.");
            }
            evaluation.records.push_back(std::move(record));
            continue;
        }

        if (record.mode == AeroExtensionMode::Active) {
            std::vector<std::string> activeOverlaps;
            for (const auto& overlap : record.metadata.overlapsWith) {
                if (std::find(context.activeBaselineMechanisms.begin(),
                              context.activeBaselineMechanisms.end(), overlap)
                    != context.activeBaselineMechanisms.end()) {
                    activeOverlaps.push_back(overlap);
                }
            }
            if (!activeOverlaps.empty()) {
                record.status = AeroExtensionStatus::Rejected;
                record.validity = AeroExtensionValidity::MethodLimited;
                for (const auto& overlap : activeOverlaps) {
                    record.diagnostics.push_back(
                        "Active baseline mechanism overlaps this extension: "
                        + overlap);
                }
                if (configuration.failurePolicy
                    == AeroExtensionFailurePolicy::Strict) {
                    throw std::runtime_error(
                        "Strict aero extension failure: overlapping body/dynamic "
                        "mechanism for " + record.metadata.id + ".");
                }
                evaluation.records.push_back(std::move(record));
                continue;
            }
        }

        const AeroExtensionApplicationMode requestedApplication =
            record.requestedApplicationMode;
        if (record.metadata.applicationMode
                == AeroExtensionApplicationMode::ComponentReplacement
            || requestedApplication
                == AeroExtensionApplicationMode::ComponentReplacement) {
            record.status = AeroExtensionStatus::Rejected;
            record.validity = AeroExtensionValidity::MethodLimited;
            record.diagnostics.push_back(
                "Component replacement is not enabled in the initial extension phase; "
                "only additive DELTA application is accepted.");
            if (configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw std::runtime_error(
                    "Strict aero extension failure: component replacement rejected for "
                    + record.metadata.id + ".");
            }
            evaluation.records.push_back(std::move(record));
            continue;
        }

        try {
            AeroExtensionContext extensionContext = context;
            extensionContext.parameters = record.configuredParameters;
            AeroExtensionResult result = extension->evaluate(
                geometry, state, baseline, extensionContext);
            record.diagnostics = std::move(result.diagnostics);
            record.diagnosticValues = std::move(result.diagnosticValues);
            record.validity = result.validity;
            if (!finite(result.delta)) {
                record.status = AeroExtensionStatus::Rejected;
                record.validity = AeroExtensionValidity::NumericallyInvalid;
                record.diagnostics.push_back(
                    "Extension returned a non-finite delta; the delta was ignored.");
            } else if (result.validity != AeroExtensionValidity::Valid) {
                record.status = AeroExtensionStatus::Rejected;
                record.delta = result.delta;
            } else if (record.mode == AeroExtensionMode::DiagnosticOnly) {
                record.status = AeroExtensionStatus::DiagnosticOnly;
                record.delta = result.delta;
            } else {
                record.status = AeroExtensionStatus::Applied;
                record.delta = result.delta;
                add(evaluation.finalResult.forceN, result.delta.forceN);
                add(evaluation.finalResult.momentNm, result.delta.momentNm);
                add(evaluation.finalResult.coefficients, result.delta.coefficients);
                evaluation.augmented = true;
            }
            if (record.status == AeroExtensionStatus::Rejected
                && configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw std::runtime_error(
                    "Strict aero extension failure: " + record.metadata.id
                    + " returned " + aeroExtensionValidityName(record.validity) + ".");
            }
        } catch (const std::exception& error) {
            if (configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw;
            }
            record.status = AeroExtensionStatus::Failed;
            record.validity = AeroExtensionValidity::InternalError;
            record.diagnostics.push_back(error.what());
        } catch (...) {
            if (configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
                throw;
            }
            record.status = AeroExtensionStatus::Failed;
            record.validity = AeroExtensionValidity::InternalError;
            record.diagnostics.push_back("Extension failed with an unknown exception.");
        }
        evaluation.records.push_back(std::move(record));
    }

    for (const auto& selected : configuration.extensions) {
        if (selected.second.mode != AeroExtensionMode::Disabled
            && registeredIds.count(selected.first) == 0) {
            evaluation.unknownEnabledExtensionIds.push_back(selected.first);
        }
    }
    if (!evaluation.unknownEnabledExtensionIds.empty()
        && configuration.failurePolicy == AeroExtensionFailurePolicy::Strict) {
        throw std::runtime_error(
            "Strict aero extension failure: unknown extension ID "
            + evaluation.unknownEnabledExtensionIds.front() + ".");
    }
    return evaluation;
}

AeroExtensionEvaluation AeroExtensionPipeline::evaluate(
    const RocketGeometry& geometry,
    const AeroState& state,
    const BaselineAeroResult& baseline,
    const AeroExtensionContext& context,
    const AeroExtensionSwitches& switches
) const {
    AeroExtensionConfiguration configuration;
    for (const auto& selected : switches) {
        configuration.extensions[selected.first].mode = selected.second
            ? AeroExtensionMode::Active : AeroExtensionMode::Disabled;
    }
    return evaluate(geometry, state, baseline, context, configuration);
}

} // namespace RoketSiM::AeroExtensions
