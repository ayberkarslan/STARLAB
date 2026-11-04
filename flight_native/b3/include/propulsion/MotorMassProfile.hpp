#pragma once

#include <vector>

#include "propulsion/MotorModel.hpp"

struct MotorMassSample {
    double time = 0.0;
    double propellantMass = 0.0;
    double totalMass = 0.0;
    double cgZ = 0.0;
    double ixx = 0.0;
    double iyy = 0.0;
    double izz = 0.0;
};

class MotorMassProfile {
public:
    MotorMassProfile(const MotorModel& motor, double length, double radius, double dt);

    const std::vector<MotorMassSample>& samples() const noexcept;

    double burnoutTime() const noexcept;
    const MotorMassSample& finalSample() const noexcept;

    MotorMassSample sampleAt(double t) const;
    double totalMassAt(double t) const;
    double iyyAt(double t) const;

private:
    MotorMassSample makeSample(double t) const;
    double computeBurnoutTime() const;

    const MotorModel& motor_;
    double length_ = 0.0;
    double radius_ = 0.0;
    double dt_ = 0.0;
    double burnoutTime_ = 0.0;
    std::vector<MotorMassSample> samples_;
};
