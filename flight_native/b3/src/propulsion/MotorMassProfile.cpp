#include "propulsion/MotorMassProfile.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

MotorMassProfile::MotorMassProfile(const MotorModel& motor, double length, double radius, double dt)
    : motor_(motor),
      length_(length),
      radius_(radius),
      dt_(dt)
{
    if (length_ <= 0.0) {
        throw std::invalid_argument("Motor mass profile length must be positive.");
    }
    if (radius_ <= 0.0) {
        throw std::invalid_argument("Motor mass profile radius must be positive.");
    }
    if (dt_ <= 0.0) {
        throw std::invalid_argument("Motor mass profile timestep must be positive.");
    }

    burnoutTime_ = computeBurnoutTime();

    for (double t = motor_.thrustCurve().burnStartTime(); t < burnoutTime_; t += dt_) {
        samples_.push_back(makeSample(t));
    }
    samples_.push_back(makeSample(burnoutTime_));
}

const std::vector<MotorMassSample>& MotorMassProfile::samples() const noexcept {
    return samples_;
}

double MotorMassProfile::burnoutTime() const noexcept {
    return burnoutTime_;
}

const MotorMassSample& MotorMassProfile::finalSample() const noexcept {
    return samples_.back();
}

MotorMassSample MotorMassProfile::sampleAt(double t) const {
    if (samples_.empty()) {
        throw std::runtime_error("Motor mass profile has no samples.");
    }
    if (t <= samples_.front().time) {
        return samples_.front();
    }
    if (t >= burnoutTime_) {
        return samples_.back();
    }

    auto upper = std::upper_bound(
        samples_.begin(),
        samples_.end(),
        t,
        [](double value, const MotorMassSample& sample) {
            return value < sample.time;
        }
    );

    const MotorMassSample& hi = *upper;
    const MotorMassSample& lo = *(upper - 1);
    const double alpha = (t - lo.time) / (hi.time - lo.time);

    MotorMassSample result;
    result.time = t;
    result.propellantMass = lo.propellantMass + alpha * (hi.propellantMass - lo.propellantMass);
    result.totalMass = lo.totalMass + alpha * (hi.totalMass - lo.totalMass);
    result.cgZ = lo.cgZ + alpha * (hi.cgZ - lo.cgZ);
    result.ixx = lo.ixx + alpha * (hi.ixx - lo.ixx);
    result.iyy = lo.iyy + alpha * (hi.iyy - lo.iyy);
    result.izz = lo.izz + alpha * (hi.izz - lo.izz);
    return result;
}

double MotorMassProfile::totalMassAt(double t) const {
    return sampleAt(t).totalMass;
}

double MotorMassProfile::iyyAt(double t) const {
    return sampleAt(t).iyy;
}

MotorMassSample MotorMassProfile::makeSample(double t) const {
    MotorMassSample sample;
    sample.time = t;
    sample.propellantMass = std::max(0.0, motor_.mass(t) - motor_.dryMass());
    sample.totalMass = motor_.dryMass() + sample.propellantMass;
    sample.cgZ = length_ / 2.0;

    sample.izz = 0.5 * sample.totalMass * radius_ * radius_;
    sample.ixx = (1.0 / 12.0) * sample.totalMass * (3.0 * radius_ * radius_ + length_ * length_);
    sample.iyy = sample.ixx;
    return sample;
}

double MotorMassProfile::computeBurnoutTime() const {
    if (motor_.initialPropellantMass() <= 0.0) {
        return motor_.thrustCurve().burnStartTime();
    }

    const double requiredImpulse = motor_.initialPropellantMass() * motor_.isp() * MotorModel::g0;
    if (motor_.thrustCurve().totalImpulse() <= requiredImpulse) {
        return motor_.thrustCurve().burnEndTime();
    }

    double lo = motor_.thrustCurve().burnStartTime();
    double hi = motor_.thrustCurve().burnEndTime();
    for (int i = 0; i < 80; ++i) {
        const double mid = 0.5 * (lo + hi);
        if (motor_.thrustCurve().impulseUntil(mid) < requiredImpulse) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    return hi;
}
