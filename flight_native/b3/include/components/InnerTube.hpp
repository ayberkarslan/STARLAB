#pragma once

#include <memory>
#include <string>

#include "components/Component.hpp"
#include "malzeme/Malzeme.h"

class InnerTube : public Component {
public:
    InnerTube(
        std::string name,
        double innerDiameter,
        double outerDiameter,
        double length,
        const Malzeme& material
    );

    double innerDiameter() const noexcept;
    double outerDiameter() const noexcept;
    double innerRadius() const noexcept;
    double outerRadius() const noexcept;
    double length() const noexcept;
    double wallThickness() const noexcept;

    double axialLength() const override;
    ComponentTrait traits() const noexcept override {
        return ComponentTrait::InternalComponent | ComponentTrait::MotorMountCapable;
    }
    bool isCompatible(const Component& child) const noexcept override {
        return hasTrait(child.traits(), ComponentTrait::InternalComponent);
    }
    double getReferenceOuterRadius() const override;

    double volume() const;
    double density() const;
    bool canFitCylinder(double radius) const noexcept;

    void addMountedComponent(std::unique_ptr<Component> child, double zOffset = 0.0);
    void printInfo() const;

protected:
    MassProperties rawMassProperties() const override;

private:
    double innerDiameter_ = 0.0;
    double outerDiameter_ = 0.0;
    double length_ = 0.0;
    Malzeme material_;
};
