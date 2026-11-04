#pragma once

#include <map>
#include <string>
#include <vector>

namespace RoketSiM::AeroExtensions {

enum class AeroExtensionMode {
    Disabled,
    DiagnosticOnly,
    Active,
};

enum class AeroExtensionApplicationMode {
    Delta,
    ComponentReplacement,
};

enum class AeroExtensionValidity {
    NotEvaluated,
    Valid,
    MethodLimited,
    OutOfRange,
    UnsupportedGeometry,
    MissingInput,
    NumericallyInvalid,
    ReferenceOnly,
    NotImplemented,
    InvalidInputStage,
    DependencyUnavailable,
    InternalError,
};

enum class AeroExtensionLifecycle {
    ReferenceOnly,
    Experimental,
    Validated,
    Production,
    Deprecated,
};

// RoketSiM body axes: +Z is axial/aft, +X/+Y are transverse.  Every public
// extension value is SI; coefficients use the reference data in AeroState.
struct BodyVector {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct RocketGeometryComponent {
    std::string id;
    std::string type;
    double axialPositionM = 0.0;
    double lengthM = 0.0;
    double radialXM = 0.0;
    double radialYM = 0.0;
    std::map<std::string, double> scalarProperties;
};

struct RocketGeometry {
    double referenceAreaM2 = 0.0;
    double referenceLengthM = 0.0;
    double referenceDiameterM = 0.0;
    double totalLengthM = 0.0;
    std::vector<RocketGeometryComponent> components;
};

struct AeroState {
    double mach = 0.0;
    double reynolds = 0.0;
    double alphaRad = 0.0;
    double betaRad = 0.0;
    double velocityMS = 0.0;
    double densityKgM3 = 0.0;
    double dynamicPressurePa = 0.0;
    double referenceAreaM2 = 0.0;
    double referenceLengthM = 0.0;
    double reynoldsReferenceLengthM = 0.0;
    BodyVector referencePointM;
    BodyVector airVelocityBodyMS;
    BodyVector angularRateBodyRadS;
    std::map<std::string, double> controlDeflectionsRad;
};

// Coefficients are additive.  normalForceSlopeMomentMPerRad is CNa * Xcp;
// carrying the first moment makes CP composable without averaging CP values.
struct AeroCoefficientSet {
    double axialForce = 0.0;
    double sideForce = 0.0;
    double normalForce = 0.0;
    double rollMoment = 0.0;
    double pitchMoment = 0.0;
    double yawMoment = 0.0;
    double drag = 0.0;
    double normalForceSlopePerRad = 0.0;
    double normalForceSlopeMomentMPerRad = 0.0;
};

struct BaselineAeroResult {
    BodyVector forceN;
    BodyVector momentNm;
    AeroCoefficientSet coefficients;
};

struct AeroExtensionDelta {
    BodyVector forceN;
    BodyVector momentNm;
    AeroCoefficientSet coefficients;
};

struct AeroExtensionContext {
    std::string baselineProvider = "internal";
    std::string analysisId;
    std::string stageId;
    std::map<std::string, double> numericInputs;
    std::vector<std::string> activeBaselineMechanisms;
    std::map<std::string, double> parameters;
};

struct AeroExtensionMetadata {
    std::string id;
    std::string displayName;
    std::string version;
    std::string family;
    std::vector<std::string> sourceMethods;
    std::vector<std::string> sourceReferences;
    int portPriority = 0;
    bool available = true;
    AeroExtensionLifecycle lifecycle = AeroExtensionLifecycle::Experimental;
    AeroExtensionApplicationMode applicationMode =
        AeroExtensionApplicationMode::Delta;
    std::string applicability;
    std::vector<std::string> supportedFlowRegimes;
    std::vector<std::string> dependencies;
    std::vector<std::string> overlapsWith;
    std::vector<std::string> limitations;
};

struct AeroExtensionResult {
    AeroExtensionDelta delta;
    AeroExtensionValidity validity = AeroExtensionValidity::Valid;
    std::vector<std::string> diagnostics;
    std::map<std::string, double> diagnosticValues;
};

class AeroExtension {
public:
    virtual ~AeroExtension() = default;

    virtual AeroExtensionResult evaluate(
        const RocketGeometry& geometry,
        const AeroState& state,
        const BaselineAeroResult& baseline,
        const AeroExtensionContext& context
    ) const = 0;

    virtual AeroExtensionMetadata metadata() const = 0;
};

} // namespace RoketSiM::AeroExtensions
