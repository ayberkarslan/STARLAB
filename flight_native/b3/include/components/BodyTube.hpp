#pragma once

#include <string>

#include "malzeme/Malzeme.h"
#include "components/Component.hpp"

class BodyTube : public Component {
public:
    BodyTube(std::string name, double outerDiameter, double thickness, double length, const Malzeme& material);

    double outerDiameter() const noexcept;
    double outerRadius() const noexcept;
    double innerRadius() const noexcept;
    double wallThickness() const noexcept;
    double length() const noexcept;

    double axialLength() const override;
    ComponentTrait traits() const noexcept override {
        return ComponentTrait::BodyComponent
            | ComponentTrait::ExternalComponent
            | ComponentTrait::MotorMountCapable;
    }
    bool isCompatible(const Component& child) const noexcept override {
        const auto childTraits = child.traits();
        return hasTrait(childTraits, ComponentTrait::ParallelStage)
            || hasTrait(childTraits, ComponentTrait::PodSet)
            || hasTrait(childTraits, ComponentTrait::InternalComponent)
            || (hasTrait(childTraits, ComponentTrait::ExternalComponent)
                && !hasTrait(childTraits, ComponentTrait::BodyComponent));
    }
    double getReferenceOuterRadius() const override;

    double volume() const;

    void printInfo() const;
    DragBreakdown dragBreakdown(const FlightCondition& fc) const override;

protected:
    MassProperties rawMassProperties() const override;
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;

private:
    double outerDiameter_ = 0.0;
    double thickness_ = 0.0;
    double length_ = 0.0;
    Malzeme material_;
};
