#pragma once

#include "components/MotorComponent.hpp"
#include "propulsion/ThrustCurve.hpp"

class MotorModel {
public:
    static constexpr double g0 = 9.80665;

    MotorModel(ThrustCurve curve, double dryMass, double propellantMass, double isp);

    const ThrustCurve& thrustCurve() const noexcept;

    double dryMass() const noexcept;
    double initialPropellantMass() const noexcept;
    double currentPropellantMass() const noexcept;
    double isp() const noexcept;

    double thrust(double t) const;
    double massFlow(double t) const;
    double mass(double t) const;
    double currentMass() const noexcept;

    void reset();
    void advance(double t, double dt);
    void syncComponent(MotorComponent& component) const;

private:
    double propellantRemainingAt(double t) const;

    ThrustCurve curve_;
    double dryMass_ = 0.0;
    double propellantMassInitial_ = 0.0;
    double currentPropellantMass_ = 0.0;
    double isp_ = 0.0;
};
