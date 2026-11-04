#pragma once
#include "estimation/StateEstimator.hpp"

namespace estimation {
// Fixed body +Z == inertial +Z. No attitude or truth enters this API.
struct VerticalConfig {
    double gravity_mps2 = 9.80665;
    double reference_pressure_pa = 101325, reference_temperature_k = 288.15;
    double lapse_k_per_m = 0.0065, gas_constant = 8314.32/28.9644;
    double earth_radius_m = 6356766;
    double pressure_noise_pa = 1.3;
    double altitude_variance_floor_m2 = 1e-6;
    double acceleration_density = 0.1, bias_walk_density = 0.01;
    double replay_window_s = 0.5;
};
struct PressureAltitude { double altitude_m, derivative_m_per_pa; };
PressureAltitude pressureAltitude(double pressure_pa, const VerticalConfig& config);

class VerticalKalman final : public StateEstimator {
public:
    explicit VerticalKalman(VerticalConfig config = {});
    void reset(const InitialState&) override;
    void consume(const sensors::Measurement&) override;
    EstimatedState state() const override;
    std::vector<Innovation> innovations() const override;
    std::size_t rejectedLate() const { return rejected_late_; }
    std::size_t replayed() const { return replayed_; }
private:
    struct Snapshot {
        double time = 0, held_acceleration = 0;
        Eigen::Vector3d x = Eigen::Vector3d::Zero();
        Eigen::Matrix3d covariance = Eigen::Matrix3d::Identity();
    };
    struct Event {
        sensors::Measurement measurement;
        Snapshot after;
        std::optional<Innovation> innovation;
    };
    VerticalConfig config_;
    Snapshot initial_, current_;
    std::vector<Event> events_;
    bool initialized_ = false;
    double last_delivery_ = -1e300;
    std::size_t rejected_late_ = 0, replayed_ = 0;
    void process(Event&, Snapshot&) const;
};
}
