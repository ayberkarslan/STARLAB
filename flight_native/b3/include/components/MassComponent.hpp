#pragma once

#include <string>

#include "components/Component.hpp"

class MassComponent : public Component {
public:
    MassComponent(std::string name, double mass, double radius, double length);

    double mass() const noexcept;
    double radius() const noexcept;
    double length() const noexcept;
    double volume() const;
    double density() const;

    double axialLength() const override;
    ComponentTrait traits() const noexcept override {
        return ComponentCategory::InternalComponent | ComponentCategory::LeafComponent;
    }
    double getReferenceOuterRadius() const override;

    void printInfo() const;

protected:
    MassProperties rawMassProperties() const override;

private:
    double mass_ = 0.0;
    double radius_ = 0.0;
    double length_ = 0.0;
};
