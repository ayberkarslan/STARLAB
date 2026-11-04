#pragma once

#include <memory>
#include <string>

#include "components/InnerTube.hpp"
#include "components/MotorConfiguration.hpp"
#include "components/MotorComponent.hpp"

class MotorMount : public InnerTube {
public:
    using InnerTube::InnerTube;

    ComponentTrait traits() const noexcept override {
        return ComponentTrait::InternalComponent | ComponentTrait::MotorMountCapable;
    }
    bool isCompatible(const Component& child) const noexcept override {
        return hasTrait(child.traits(), ComponentTrait::Motor)
            || hasTrait(child.traits(), ComponentTrait::InternalComponent);
    }

    void configure(MotorConfiguration configuration);
    const MotorConfiguration* configuration() const noexcept;
    void installMotor(std::unique_ptr<MotorComponent> motor, double zOffset = 0.0);

private:
    std::unique_ptr<MotorConfiguration> configuration_;
};
