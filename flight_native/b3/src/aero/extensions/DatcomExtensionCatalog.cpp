#include "aero/extensions/DatcomExtensionCatalog.hpp"

#include <memory>
#include <utility>

namespace RoketSiM::AeroExtensions {
namespace {

class CataloguedDatcomExtension final : public AeroExtension {
public:
    explicit CataloguedDatcomExtension(AeroExtensionMetadata metadata)
        : metadata_(std::move(metadata)) {}

    AeroExtensionResult evaluate(
        const RocketGeometry&,
        const AeroState&,
        const BaselineAeroResult&,
        const AeroExtensionContext&
    ) const override {
        AeroExtensionResult result;
        result.validity = metadata_.lifecycle == AeroExtensionLifecycle::ReferenceOnly
            ? AeroExtensionValidity::ReferenceOnly
            : AeroExtensionValidity::NotImplemented;
        result.diagnostics.push_back(
            "DATCOM/NACA source method is not yet ported or validated.");
        return result;
    }

    AeroExtensionMetadata metadata() const override { return metadata_; }

private:
    AeroExtensionMetadata metadata_;
};

AeroExtensionMetadata entry(
    const char* id,
    const char* displayName,
    std::vector<std::string> methods,
    const char* family,
    int priority,
    const char* applicability,
    AeroExtensionLifecycle lifecycle = AeroExtensionLifecycle::ReferenceOnly,
    std::vector<std::string> references = {},
    std::vector<std::string> supportedRegimes = {},
    std::vector<std::string> dependencies = {},
    std::vector<std::string> overlapsWith = {}
) {
    AeroExtensionMetadata metadata;
    metadata.id = id;
    metadata.displayName = displayName;
    metadata.version = "catalogue-2";
    metadata.family = family;
    metadata.sourceMethods = std::move(methods);
    metadata.sourceReferences = std::move(references);
    metadata.portPriority = priority;
    metadata.available = false;
    metadata.lifecycle = lifecycle;
    metadata.applicationMode = AeroExtensionApplicationMode::Delta;
    metadata.applicability = applicability;
    metadata.supportedFlowRegimes = std::move(supportedRegimes);
    metadata.dependencies = std::move(dependencies);
    metadata.overlapsWith = std::move(overlapsWith);
    metadata.limitations = {
        "OFF by default.",
        "No production equation is registered until source port and validation are reviewed.",
    };
    return metadata;
}

} // namespace

AeroExtensionPipeline makeDatcomExtensionPipeline() {
    std::vector<std::shared_ptr<const AeroExtension>> extensions;
    const auto add = [&](AeroExtensionMetadata metadata) {
        extensions.push_back(std::make_shared<CataloguedDatcomExtension>(
            std::move(metadata)));
    };

    add(entry("datcom_dynbod", "DATCOM DYNBOD", {"DYNBOD"},
              "body_dynamics", 1, "DATCOM body dynamic derivatives.",
              AeroExtensionLifecycle::Experimental,
              {"Digital DATCOM DYNBOD source archaeology"},
              {"SUBSONIC", "TRANSONIC", "SUPERSONIC"}, {},
              {"distributed_body_damping", "dynamic_fin_model"}));
    add(entry("naca1307_fin_body", "NACA 1307 fin/body interference",
              {"NACA-TR-1307"}, "fin_body_interference", 1,
              "Fin-in-body, fin-alone, and body-due-to-fin decomposition.",
              AeroExtensionLifecycle::Experimental,
              {"NACA Technical Report 1307"}, {"SUBSONIC"}));
    add(entry("datcom_wblift", "DATCOM WBLIFT", {"WBLIFT"},
              "fin_body_interference", 1, "Wing/body lift interference.",
              AeroExtensionLifecycle::Experimental));
    add(entry("datcom_wbaero", "DATCOM WBAERO", {"WBAERO"},
              "fin_body_interference", 1, "Wing/body aerodynamic interference.",
              AeroExtensionLifecycle::Experimental));
    add(entry("datcom_supwb", "DATCOM SUPWB", {"SUPWB"},
              "supersonic_interference", 1, "Supersonic wing/body interference.",
              AeroExtensionLifecycle::Experimental, {}, {"SUPERSONIC"},
              {"datcom_intkbw"}));
    add(entry("datcom_intkbw", "DATCOM INTKBW", {"INTKBW"},
              "supersonic_interference", 1,
              "Supersonic body/wing interference factors.",
              AeroExtensionLifecycle::Experimental, {}, {"SUPERSONIC"}));

    add(entry("datcom_bodyrt", "DATCOM BODYRT", {"BODYRT"},
              "body_aerodynamics", 2, "Body aerodynamic method."));
    add(entry("datcom_bodyjm", "DATCOM BODYJM", {"BODYJM"},
              "body_aerodynamics", 2, "Body aerodynamic method."));
    add(entry("datcom_sypbod", "DATCOM SYPBOD", {"SYPBOD"},
              "body_aerodynamics", 2, "Body aerodynamic synthesis."));
    add(entry("datcom_liftfp", "DATCOM LIFTFP", {"LIFTFP"},
              "control_surface", 2, "Fin/control-surface lift."));
    add(entry("datcom_flapcm", "DATCOM FLAPCM", {"FLAPCM"},
              "control_surface", 2, "Flap pitching moment."));
    add(entry("datcom_gdelta", "DATCOM GDELTA", {"GDELTA"},
              "control_surface", 2, "Control-effectiveness correction."));
    add(entry("datcom_suplng", "DATCOM SUPLNG", {"SUPLNG"},
              "supersonic_longitudinal", 2,
              "Supersonic longitudinal aerodynamics.",
              AeroExtensionLifecycle::ReferenceOnly, {}, {"SUPERSONIC"}));
    add(entry("datcom_supcld", "DATCOM SUPCLD", {"SUPCLD"},
              "supersonic_damping", 2, "Supersonic lift damping.",
              AeroExtensionLifecycle::ReferenceOnly, {}, {"SUPERSONIC"}));
    add(entry("datcom_supcmd", "DATCOM SUPCMD", {"SUPCMD"},
              "supersonic_damping", 2, "Supersonic moment damping.",
              AeroExtensionLifecycle::ReferenceOnly, {}, {"SUPERSONIC"}));
    add(entry("datcom_dnpawb", "DATCOM DNPAWB", {"DNPAWB"},
              "dynamic_interference", 2, "Wing/body dynamic interference."));

    return AeroExtensionPipeline(std::move(extensions));
}

AeroExtensionSwitches defaultDatcomExtensionSwitches() {
    AeroExtensionSwitches switches;
    const AeroExtensionPipeline pipeline = makeDatcomExtensionPipeline();
    for (const auto& extension : pipeline.extensions()) {
        switches.emplace(extension->metadata().id, false);
    }
    return switches;
}

AeroExtensionConfiguration defaultDatcomExtensionConfiguration() {
    AeroExtensionConfiguration configuration;
    const AeroExtensionPipeline pipeline = makeDatcomExtensionPipeline();
    for (const auto& extension : pipeline.extensions()) {
        configuration.extensions.emplace(
            extension->metadata().id, AeroExtensionSelection{});
    }
    return configuration;
}

} // namespace RoketSiM::AeroExtensions
