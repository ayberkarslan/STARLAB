#include "estimation/ValidationRunner.hpp"
#include <fstream>
#include <set>
#include <stdexcept>

namespace estimation::validation {
std::vector<Scenario> loadScenarios(const std::string& path) {
    std::ifstream input(path);
    if(!input) throw std::runtime_error("cannot open scenario configuration: "+path);
    nlohmann::json root; input>>root;
    if(root.at("schema_version")!=1) throw std::invalid_argument("unsupported scenario schema");
    const std::set<std::string> allowed={"accel_rate_hz","accel_density","gyro_density","pressure_rms_pa",
        "accel_bias","accel_walk","gyro_walk","vibration_hz","vibration_amplitude","cutoff_hz",
        "full_scale_mps2","baro_dropout","gnss_dropout","baro_latency_s","accel_jitter_s"};
    std::vector<Scenario> out;
    std::set<std::string> names;
    for(const auto& row:root.at("scenarios")) {
        Scenario scenario; scenario.name=row.at("name"); scenario.description=row.at("description");
        if(!names.insert(scenario.name).second) throw std::invalid_argument("duplicate scenario name");
        auto c=root.at("defaults"); c.update(row.at("overrides"));
        for(auto it=c.begin();it!=c.end();++it)
            if(!allowed.count(it.key()) || !it.value().is_number()) throw std::invalid_argument("invalid scenario field: "+it.key());
        scenario.resolved_config=c;
        auto& s=scenario.sensors; s.seed=20260905;
        s.accelerometer.clock.rate_hz=c.at("accel_rate_hz"); s.accelerometer.internal_rate_hz=2000;
        s.gyroscope.clock.rate_hz=100; s.gyroscope.internal_rate_hz=1000;
        s.accelerometer.stochastic.white_density=sensors::Vec3::Constant(c.at("accel_density").get<double>());
        s.accelerometer.stochastic.white_enabled=c.at("accel_density").get<double>()>0;
        s.gyroscope.stochastic.white_density=sensors::Vec3::Constant(c.at("gyro_density").get<double>());
        s.gyroscope.stochastic.white_enabled=c.at("gyro_density").get<double>()>0;
        s.accelerometer.calibration.bias_enabled=c.at("accel_bias").get<double>()!=0;
        s.accelerometer.calibration.bias.z()=c.at("accel_bias");
        s.accelerometer.stochastic.random_walk_enabled=c.at("accel_walk").get<double>()>0;
        s.accelerometer.stochastic.random_walk.z()=c.at("accel_walk");
        s.gyroscope.stochastic.random_walk_enabled=c.at("gyro_walk").get<double>()>0;
        s.gyroscope.stochastic.random_walk=sensors::Vec3::Constant(c.at("gyro_walk").get<double>());
        s.accelerometer.clock.jitter_std_s=c.at("accel_jitter_s");
        s.accelerometer.transfer.cutoff_hz=c.at("cutoff_hz");
        const double range=c.at("full_scale_mps2");
        if(range>0) {s.accelerometer.saturation_enabled=true; s.accelerometer.full_scale=sensors::Vec3::Constant(range);}
        if(c.at("vibration_amplitude").get<double>()>0) {
            sensors::VibrationProfile vibration; vibration.mode=sensors::VibrationMode::SINUSOIDAL;
            vibration.harmonics.push_back({c.at("vibration_hz"),sensors::Vec3(0,0,c.at("vibration_amplitude")),sensors::Vec3::Zero()});
            s.accelerometer.phase_vibration[sensors::FlightPhase::POWERED_ASCENT]=vibration;
        }
        s.barometer=sensors::BarometerProfile{}; s.barometer->clock.rate_hz=10;
        s.barometer->noise_rms_pa=c.at("pressure_rms_pa");
        s.barometer->clock.dropout_probability=c.at("baro_dropout");
        s.barometer->clock.latency_s=c.at("baro_latency_s");
        s.gnss=sensors::GnssProfile{}; s.gnss->clock.rate_hz=5;
        s.gnss->clock.dropout_probability=c.at("gnss_dropout");
        // Identical declared tuning across stress scenarios; no truth-derived retuning.
        scenario.estimator=VerticalConfig{};
        s.accelerometer.validate(); s.gyroscope.validate(); s.barometer->clock.validate(); s.gnss->clock.validate();
        for(auto it=c.begin();it!=c.end();++it)
            if(!std::isfinite(it.value().get<double>()) || (it.key()!="accel_bias" && it.value().get<double>()<0))
                throw std::invalid_argument("invalid scenario numeric value");
        out.push_back(std::move(scenario));
    }
    if(out.empty()) throw std::invalid_argument("empty scenarios");
    return out;
}
}
