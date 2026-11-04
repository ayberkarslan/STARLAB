#include "estimation/VerticalKalman.hpp"
#include <Eigen/Cholesky>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace estimation {
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::invalid_argument(message);
}
sensors::Stamp stamp(const sensors::Measurement& m) {
    return std::visit([](const auto& sample) { return sample.stamp; }, m.sample);
}
bool before(const sensors::Measurement& a, const sensors::Measurement& b) {
    const auto sa = stamp(a), sb = stamp(b);
    if (sa.truth_timestamp != sb.truth_timestamp) return sa.truth_timestamp < sb.truth_timestamp;
    if (a.kind != b.kind) return int(a.kind) < int(b.kind);
    return sa.sequence < sb.sequence;
}
void validate(const VerticalConfig& c) {
    for (double v : {c.gravity_mps2,c.reference_pressure_pa,c.reference_temperature_k,
                     c.lapse_k_per_m,c.gas_constant,c.earth_radius_m,c.altitude_variance_floor_m2})
        require(std::isfinite(v) && v > 0, "positive finite vertical model parameter required");
    for (double v : {c.pressure_noise_pa,c.acceleration_density,c.bias_walk_density,c.replay_window_s})
        require(std::isfinite(v) && v >= 0, "nonnegative finite vertical model parameter required");
}
}
PressureAltitude pressureAltitude(double pressure, const VerticalConfig& c) {
    validate(c);
    require(std::isfinite(pressure) && pressure > 0, "pressure must be positive finite Pa");
    // Inverse of the tropospheric hydrostatic lapse law; geopotential -> geometric height.
    const double exponent = c.gas_constant*c.lapse_k_per_m/c.gravity_mps2;
    const double ratio = std::pow(pressure/c.reference_pressure_pa, exponent);
    const double geopotential = c.reference_temperature_k/c.lapse_k_per_m*(1-ratio);
    require(geopotential >= -600 && geopotential <= 11000, "pressure outside tropospheric observation model");
    const double scale = c.earth_radius_m/(c.earth_radius_m-geopotential);
    return {scale*geopotential,
            -scale*scale*c.reference_temperature_k/c.lapse_k_per_m*exponent*ratio/pressure};
}
VerticalKalman::VerticalKalman(VerticalConfig c) : config_(c) { validate(c); }
void VerticalKalman::reset(const InitialState& s) {
    require(std::isfinite(s.timestamp) && std::isfinite(s.altitude_m) &&
            std::isfinite(s.vertical_velocity_mps) && std::isfinite(s.accel_bias_mps2), "invalid initial state");
    require(s.covariance.allFinite() && s.covariance.isApprox(s.covariance.transpose(),1e-12), "invalid initial covariance");
    Eigen::LLT<Eigen::Matrix3d> llt(s.covariance);
    require(llt.info()==Eigen::Success, "initial covariance must be positive definite");
    initial_.time=s.timestamp; initial_.x=Eigen::Vector3d(s.altitude_m,s.vertical_velocity_mps,s.accel_bias_mps2);
    initial_.covariance=s.covariance; initial_.held_acceleration=0;
    current_=initial_; events_.clear(); initialized_=true;
    last_delivery_=-1e300; rejected_late_=0; replayed_=0;
}
void VerticalKalman::process(Event& e, Snapshot& s) const {
    const auto st=stamp(e.measurement);
    const double dt=st.truth_timestamp-s.time;
    require(dt>=0, "negative prediction interval");
    const double d2=dt*dt, d3=d2*dt, d4=d3*dt, d5=d4*dt;
    Eigen::Matrix3d f=Eigen::Matrix3d::Identity();
    f(0,1)=dt; f(0,2)=-0.5*d2; f(1,2)=-dt;
    s.x=f*s.x+Eigen::Vector3d(0.5*d2,dt,0)*s.held_acceleration;
    // Exact continuous white acceleration + bias Brownian motion discretization.
    Eigen::Matrix3d qa=Eigen::Matrix3d::Zero(), qb;
    qa(0,0)=d3/3; qa(0,1)=qa(1,0)=d2/2; qa(1,1)=dt;
    qb << d5/20,d4/8,-d3/6, d4/8,d3/3,-d2/2, -d3/6,-d2/2,dt;
    s.covariance=f*s.covariance*f.transpose()+
        config_.acceleration_density*config_.acceleration_density*qa+
        config_.bias_walk_density*config_.bias_walk_density*qb;
    s.time=st.truth_timestamp; e.innovation.reset();
    if(e.measurement.kind==sensors::SensorKind::Accelerometer) {
        const auto& sample=std::get<sensors::VectorSample>(e.measurement.sample);
        s.held_acceleration=sample.value.z()-config_.gravity_mps2;
    } else {
        const auto z=pressureAltitude(std::get<sensors::PressureSample>(e.measurement.sample).pressure_pa,config_);
        const double r=std::max(config_.altitude_variance_floor_m2,
            z.derivative_m_per_pa*z.derivative_m_per_pa*config_.pressure_noise_pa*config_.pressure_noise_pa);
        const double residual=z.altitude_m-s.x[0], variance=s.covariance(0,0)+r;
        const Eigen::Vector3d gain=s.covariance.col(0)/variance;
        s.x+=gain*residual;
        Eigen::Matrix3d correction=Eigen::Matrix3d::Identity(); correction.col(0)-=gain;
        s.covariance=correction*s.covariance*correction.transpose()+r*gain*gain.transpose(); // Joseph form
        e.innovation=Innovation{st.truth_timestamp,st.measurement_timestamp,residual,variance,residual*residual/variance};
    }
    s.covariance=(0.5*(s.covariance+s.covariance.transpose())).eval();
    if(!s.x.allFinite() || !s.covariance.allFinite()) throw std::runtime_error("nonfinite Kalman state");
    e.after=s;
}
void VerticalKalman::consume(const sensors::Measurement& m) {
    require(initialized_, "reset estimator before consume");
    if(m.kind!=sensors::SensorKind::Accelerometer && m.kind!=sensors::SensorKind::Barometer) return;
    const auto st=stamp(m);
    require(std::isfinite(st.truth_timestamp) && std::isfinite(st.measurement_timestamp), "nonfinite measurement timestamp");
    require(st.measurement_timestamp>=last_delivery_, "measurements must arrive in delivery order");
    if(m.kind==sensors::SensorKind::Accelerometer) {
        require(std::holds_alternative<sensors::VectorSample>(m.sample), "accelerometer variant mismatch");
        require(std::get<sensors::VectorSample>(m.sample).value.allFinite(), "nonfinite accelerometer");
    } else {
        require(std::holds_alternative<sensors::PressureSample>(m.sample), "barometer variant mismatch");
        pressureAltitude(std::get<sensors::PressureSample>(m.sample).pressure_pa,config_);
    }
    last_delivery_=st.measurement_timestamp;
    if(st.truth_timestamp<initial_.time || current_.time-st.truth_timestamp>config_.replay_window_s) {
        ++rejected_late_; return;
    }
    auto it=std::lower_bound(events_.begin(),events_.end(),m,[](const Event& e,const auto& value){return before(e.measurement,value);});
    if(it!=events_.end() && !before(m,it->measurement)) return; // duplicate key
    const auto index=std::size_t(it-events_.begin());
    if(index<events_.size()) ++replayed_;
    events_.insert(it,Event{m,{}, {}});
    Snapshot s=index==0?initial_:events_[index-1].after;
    for(std::size_t i=index;i<events_.size();++i) process(events_[i],s);
    current_=s;
}
EstimatedState VerticalKalman::state() const {
    if(!initialized_) return {};
    return {current_.time,current_.x[0],current_.x[1],current_.x[2],current_.covariance,true,true};
}
std::vector<Innovation> VerticalKalman::innovations() const {
    std::vector<Innovation> out;
    for(const auto& e:events_) if(e.innovation) out.push_back(*e.innovation);
    return out;
}
}
