#include "propulsion/MotorModel.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

MotorModel::MotorModel(ThrustCurve curve, double dryMass, double propellantMass, double isp)
    : curve_(std::move(curve)),
      dryMass_(dryMass),
      propellantMassInitial_(propellantMass),
      currentPropellantMass_(propellantMass),
      isp_(isp)
{
    if (dryMass_ < 0.0) {
        throw std::invalid_argument("Motor dry mass must be non-negative.");
    }
    if (propellantMassInitial_ < 0.0) {
        throw std::invalid_argument("Motor propellant mass must be non-negative.");
    }
    if ((dryMass_ + propellantMassInitial_) <= 0.0) {
        throw std::invalid_argument("Motor total mass must be positive.");
    }
    if (isp_ <= 0.0) {
        throw std::invalid_argument("Motor Isp must be positive.");
    }
}

const ThrustCurve& MotorModel::thrustCurve() const noexcept {
    return curve_;
}

double MotorModel::dryMass() const noexcept {
    return dryMass_;
}

double MotorModel::initialPropellantMass() const noexcept {
    return propellantMassInitial_;
}

double MotorModel::currentPropellantMass() const noexcept {
    return currentPropellantMass_;
}

double MotorModel::isp() const noexcept {
    return isp_;
}

double MotorModel::thrust(double t) const {
    return curve_.getThrust(t);
}

double MotorModel::massFlow(double t) const {
    return thrust(t) / (isp_ * g0);
}

double MotorModel::mass(double t) const {
    return dryMass_ + propellantRemainingAt(t);
}

double MotorModel::currentMass() const noexcept {
    return dryMass_ + currentPropellantMass_;
}

void MotorModel::reset() {
    currentPropellantMass_ = propellantMassInitial_;
}

void MotorModel::advance(double t, double dt) {
    if (dt < 0.0) {
        throw std::invalid_argument("MotorModel timestep must be non-negative.");
    }

    const double burned = curve_.impulseBetween(t, t + dt) / (isp_ * g0);
    currentPropellantMass_ = std::max(0.0, currentPropellantMass_ - burned);
}

void MotorModel::syncComponent(MotorComponent& component) const {
    component.setPropellantMass(currentPropellantMass_);
}

double MotorModel::propellantRemainingAt(double t) const {
    const double burned = curve_.impulseUntil(t) / (isp_ * g0);
    return std::max(0.0, propellantMassInitial_ - burned);
}
