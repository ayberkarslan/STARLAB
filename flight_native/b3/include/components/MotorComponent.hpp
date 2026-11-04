#pragma once

#include <string>

#include "components/Component.hpp"

class MotorComponent : public Component {
public:
    MotorComponent(
        std::string name,
        double dryMass,
        double propellantMass,
        double length,
        double radius
    );

    double dryMass() const noexcept;
    double propellantMass() const noexcept;
    double totalMass() const noexcept;
    double length() const noexcept;
    double radius() const noexcept;

    void setPropellantMass(double propellantMass);

    double axialLength() const override;
    ComponentTrait traits() const noexcept override {
        return ComponentCategory::Motor | ComponentCategory::LeafComponent;
    }
    double getReferenceOuterRadius() const override;

protected:
    MassProperties rawMassProperties() const override;

private:
    double dryMass_ = 0.0;
    double propellantMass_ = 0.0;
    double length_ = 0.0;
    double radius_ = 0.0;
};
