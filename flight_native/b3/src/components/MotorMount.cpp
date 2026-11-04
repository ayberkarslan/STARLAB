#include <utility>

#include "components/MotorMount.hpp"

void MotorMount::configure(MotorConfiguration configuration) {
    configuration.validate();
    configuration_ = std::make_unique<MotorConfiguration>(std::move(configuration));
}

const MotorConfiguration* MotorMount::configuration() const noexcept {
    return configuration_.get();
}

void MotorMount::installMotor(std::unique_ptr<MotorComponent> motor, double zOffset) {
    if (!motor) {
        throw std::invalid_argument("Motor mount cannot install a null motor.");
    }
    addMountedComponent(std::move(motor), zOffset);
}
