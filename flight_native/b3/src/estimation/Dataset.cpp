#include "estimation/ValidationRunner.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace estimation::validation {
void exportDataset(const std::string& directory,const FlightFixture& fixture,const Scenario& scenario,
                   const sensors::SyntheticSensorRun& run,const EstimatorResult& result) {
    namespace fs=std::filesystem;
    fs::create_directories(directory);
    auto file=[&](const char* name) {
        std::ofstream stream(fs::path(directory)/name);
        if(!stream) throw std::runtime_error("cannot write dataset file");
        stream.exceptions(std::ios::badbit|std::ios::failbit);
        stream<<std::setprecision(17); return stream;
    };
    auto truth=file("truth.csv");
    truth<<"time,x,y,z,vx,vy,vz,ax,ay,az,qw,qx,qy,qz,wx,wy,wz,gx,gy,gz,pressure_pa,temperature_k,phase\n";
    for(const auto& t:fixture.samples) {
        truth<<t.time;
        for(const auto& v:{t.position,t.velocity,t.acceleration}) for(int i=0;i<3;++i) truth<<','<<v[i];
        truth<<','<<t.body_to_inertial.w()<<','<<t.body_to_inertial.x()<<','<<t.body_to_inertial.y()<<','<<t.body_to_inertial.z();
        for(const auto& v:{t.angular_rate_body,t.gravity}) for(int i=0;i<3;++i) truth<<','<<v[i];
        truth<<','<<t.pressure_pa<<','<<t.temperature_k<<','<<int(t.phase)<<'\n';
    }
    auto measurements=file("measurements.csv");
    measurements<<"kind,truth_timestamp,measurement_timestamp,sequence,x,y,z,vx,vy,vz,sat_x,sat_y,sat_z\n";
    std::vector<std::size_t> counts(4,0);
    for(const auto& m:run.measurements) {
        const auto st=std::visit([](const auto& s){return s.stamp;},m.sample);
        sensors::Vec3 value=sensors::Vec3::Zero(),velocity=sensors::Vec3::Zero(),flags=sensors::Vec3::Zero();
        std::visit([&](const auto& sample) {
            using T=std::decay_t<decltype(sample)>;
            if constexpr(std::is_same_v<T,sensors::VectorSample>) {value=sample.value;flags=sample.saturated.template cast<double>().matrix();}
            else if constexpr(std::is_same_v<T,sensors::PressureSample>) value.x()=sample.pressure_pa;
            else {value=sample.position_m;velocity=sample.velocity_mps;}
        },m.sample);
        measurements<<int(m.kind)<<','<<st.truth_timestamp<<','<<st.measurement_timestamp<<','<<st.sequence;
        for(const auto& v:{value,velocity,flags}) for(int i=0;i<3;++i) measurements<<','<<v[i];
        measurements<<'\n'; ++counts.at(int(m.kind));
    }
    auto estimates=file("estimates.csv");
    estimates<<"delivery_timestamp,state_timestamp,h,v,b,p00,p01,p02,p10,p11,p12,p20,p21,p22,initialized,valid,h_error,v_error,nees_hv\n";
    for(const auto& row:result.history) {
        const auto& s=row.state;
        if(!s.accel_bias_mps2 || !s.covariance) throw std::invalid_argument("vertical dataset requires bias and covariance");
        estimates<<row.delivery_timestamp<<','<<s.timestamp<<','<<s.altitude_m<<','<<s.vertical_velocity_mps<<','<<*s.accel_bias_mps2;
        for(int i=0;i<3;++i) for(int j=0;j<3;++j) estimates<<','<<(*s.covariance)(i,j);
        estimates<<','<<s.initialized<<','<<s.valid<<','<<row.altitude_error_m<<','<<row.velocity_error_mps<<','<<row.nees_hv<<'\n';
    }
    auto innovations=file("innovations.csv");
    innovations<<"truth_timestamp,measurement_timestamp,residual_m,covariance_m2,nis\n";
    for(const auto& x:result.innovations) innovations<<x.timestamp<<','<<x.delivery_timestamp<<','<<x.residual_m<<','<<x.covariance_m2<<','<<x.nis<<'\n';
    const auto& e=scenario.estimator;
    nlohmann::json metadata={
        {"schema_version",1},{"generator_version","estimator-validation-1"},{"source_flight_fixture",fixture.source},
        {"seed",scenario.sensors.seed},{"scenario",scenario.name},{"sensor_configuration",scenario.resolved_config},
        {"sensor_profiles",{"CUSTOM","CUSTOM","CUSTOM","CUSTOM"}},
        {"fixed_sensor_configuration",{{"accel_internal_hz",2000},{"gyro_internal_hz",1000},{"gyro_odr_hz",100},{"baro_odr_hz",10},{"gnss_odr_hz",5},
          {"iir_coefficient",1},{"quantization",false},{"gm",false},{"digital_filter",false},{"scale_error",{0,0,0}},
          {"misalignment","identity"},{"gnss_bias_and_noise",0},{"baro_bias_pa",0},{"filter_order",1},{"phase_profile","POWERED_ASCENT only; elapsed-run phase"}}},
        {"estimator_configuration",{{"gravity_mps2",e.gravity_mps2},{"reference_pressure_pa",e.reference_pressure_pa},
          {"reference_temperature_k",e.reference_temperature_k},{"lapse_k_per_m",e.lapse_k_per_m},{"gas_constant",e.gas_constant},
          {"earth_radius_m",e.earth_radius_m},{"pressure_noise_pa",e.pressure_noise_pa},{"altitude_variance_floor_m2",e.altitude_variance_floor_m2},
          {"acceleration_density",e.acceleration_density},{"bias_walk_density",e.bias_walk_density},{"replay_window_s",e.replay_window_s},
          {"initial_h_v_b",{0,0,0}},{"initial_covariance_diagonal",{25,25,1}}}},
        {"coordinate_frame","right-handed local frame, +Z up; body aligned, identity body-to-inertial quaternion"},
        {"units",{{"time","s"},{"position","m"},{"velocity","m/s"},{"acceleration_bias","m/s^2"},{"angular_rate","rad/s"},{"pressure","Pa"},{"temperature","K"}}},
        {"kind_codes",{"accelerometer","gyroscope","barometer","gnss"}},
        {"pressure_csv_storage","pressure_pa is measurements.x for kind=2; unused columns zero"},
        {"expected_sample_counts",counts},{"truth_count",fixture.samples.size()},{"estimate_count",result.history.size()},
        {"innovation_count",result.innovations.size()},{"metrics",metricsJson(result.metrics)},
        {"fixture",{{"dt_s",.01},{"duration_s",8},{"mass_kg",10},{"powered_net_force_N",20},{"ignition_s",1},{"burnout_s",6}}}
    };
    auto meta=file("metadata.json"); meta<<metadata.dump(2)<<'\n';
}
}
