#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "components/MotorComponent.hpp"

MotorComponent::MotorComponent(
    std::string name,
    double dryMass,
    double propellantMass,
    double length,
    double radius
)
    : dryMass_(dryMass),
      propellantMass_(propellantMass),
      length_(length),
      radius_(radius)
{
    this->name_ = std::move(name);

    if (dryMass_ < 0.0) {
        throw std::invalid_argument("Motor dry mass must be non-negative.");
    }
    if (propellantMass_ < 0.0) {
        throw std::invalid_argument("Motor propellant mass must be non-negative.");
    }
    if ((dryMass_ + propellantMass_) <= 0.0) {
        throw std::invalid_argument("Motor total mass must be positive.");
    }
    if (length_ <= 0.0) {
        throw std::invalid_argument("Motor length must be positive.");
    }
    if (radius_ <= 0.0) {
        throw std::invalid_argument("Motor radius must be positive.");
    }
}

double MotorComponent::dryMass() const noexcept {
    return dryMass_;
}

double MotorComponent::propellantMass() const noexcept {
    return propellantMass_;
}

double MotorComponent::totalMass() const noexcept {
    return dryMass_ + propellantMass_;
}

double MotorComponent::length() const noexcept {
    return length_;
}

double MotorComponent::radius() const noexcept {
    return radius_;
}

void MotorComponent::setPropellantMass(double propellantMass) {
    if (propellantMass < 0.0) {
        throw std::invalid_argument("Motor propellant mass must be non-negative.");
    }
    propellantMass_ = propellantMass;
}

double MotorComponent::axialLength() const {
    return length_;
}

double MotorComponent::getReferenceOuterRadius() const {
    return radius_;
}

MassProperties MotorComponent::rawMassProperties() const {
    MassProperties props;
    props.mass = totalMass();
    props.cgLocal = Eigen::Vector3d(0.0, 0.0, length_ / 2.0);

    const double Izz = 0.5 * props.mass * radius_ * radius_;
    const double Ixx = (1.0 / 12.0) * props.mass * (3.0 * radius_ * radius_ + length_ * length_);

    props.inertiaLocal(0, 0) = Ixx;
    props.inertiaLocal(1, 1) = Ixx;
    props.inertiaLocal(2, 2) = Izz;
    return props;
}
