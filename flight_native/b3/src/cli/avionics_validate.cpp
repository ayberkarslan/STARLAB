// Desktop transport adapter. All scientific calculations delegate to validated libraries.
#include "estimation/ValidationRunner.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using nlohmann::json;
using namespace sensors;
using namespace estimation::validation;
namespace {
Vec3 vec(const json& j) { if(!j.is_array()||j.size()!=3)throw std::invalid_argument("three SI axes required");return {j[0].get<double>(),j[1].get<double>(),j[2].get<double>()}; }
json arr(const Vec3& v) {return {v.x(),v.y(),v.z()};}
void clockConfig(SensorClock& c,const json& j) {
    c.rate_hz=j.value("rate_hz",c.rate_hz);c.latency_s=j.value("latency_s",c.latency_s);
    c.jitter_std_s=j.value("jitter_s",c.jitter_std_s);c.dropout_probability=j.value("dropout",c.dropout_probability);c.validate();
}
VibrationProfile vibration(const json& j) {
    VibrationProfile p;const std::string mode=j.value("mode","OFF");
    if(mode=="OFF")return p;
    if(mode=="RANDOM_RMS") {p.mode=VibrationMode::RANDOM_RMS;p.rms=vec(j.at("rms"));}
    else if(mode=="SINUSOIDAL") {p.mode=VibrationMode::SINUSOIDAL;for(const auto& h:j.at("harmonics"))if(h.value("enabled",true))p.harmonics.push_back({h.at("frequency_hz"),vec(h.at("amplitude")),vec(h.at("phase_rad"))});}
    else if(mode=="PSD") {p.mode=VibrationMode::PSD;std::ifstream in(j.at("path").get<std::string>());p.psd=importPsdCsv(in,j.value("g_squared",false));}
    else if(mode=="RECORDED") {p.mode=VibrationMode::RECORDED;std::ifstream in(j.at("path").get<std::string>());auto data=importRecordedCsv(in,j.value("remove_dc",true),j.value("detrend",false));p.recorded=j.value("channel","accel")=="gyro"?data.gyro:data.accel;}
    else throw std::invalid_argument("unknown vibration mode");
    return p;
}
void sensor(SensorProfile& p,const json& j,bool gyro) {
    const auto profile=j.value("profile","SCENARIO");
    if(profile=="IDEAL")p=SensorProfile{};
    else if(profile=="BMI088")p=gyro?bmi088Gyroscope():bmi088Accelerometer();
    else if(profile=="MPU6500")p=gyro?mpu6500Gyroscope():mpu6500Accelerometer();
    else if(profile=="CHARACTERIZED") {std::ifstream in(j.at("path").get<std::string>());p=importCharacterizedProfile(in);}
    else if(profile!="SCENARIO"&&profile!="CUSTOM")throw std::invalid_argument("unsupported sensor profile");
    if(j.contains("clock"))clockConfig(p.clock,j.at("clock"));
    p.internal_rate_hz=j.value("internal_rate_hz",p.internal_rate_hz);
    if(j.contains("full_scale")){p.full_scale=vec(j.at("full_scale"));p.saturation_enabled=true;}
    p.saturation_enabled=j.value("saturation",p.saturation_enabled);
    if(j.contains("white_density")){p.stochastic.white_density=vec(j.at("white_density"));p.stochastic.white_enabled=true;}
    if(j.contains("bias")){p.calibration.bias=vec(j.at("bias"));p.calibration.bias_enabled=true;}
    if(j.contains("random_walk")){p.stochastic.random_walk=vec(j.at("random_walk"));p.stochastic.random_walk_enabled=true;}
    if(j.contains("gm_sigma")){p.stochastic.gm_sigma=vec(j.at("gm_sigma"));p.stochastic.gm_tau_s=vec(j.at("gm_tau_s"));p.stochastic.gm_enabled=true;}
    if(j.contains("bits"))p.setResolution(j.at("bits"));
    p.quantization_enabled=j.value("quantization",p.quantization_enabled);
    p.transfer.cutoff_hz=j.value("cutoff_hz",p.transfer.cutoff_hz);p.transfer.order=j.value("filter_order",p.transfer.order);
    p.digital_transfer.cutoff_hz=j.value("digital_cutoff_hz",p.digital_transfer.cutoff_hz);p.digital_transfer.order=j.value("digital_order",p.digital_transfer.order);
    if(j.contains("vibration")){p.vibration=vibration(j.at("vibration"));p.phase_vibration.clear();}
    if(j.contains("phase_vibration"))for(auto it=j.at("phase_vibration").begin();it!=j.at("phase_vibration").end();++it){int phase=std::stoi(it.key());if(phase<0||phase>7)throw std::invalid_argument("invalid phase");p.phase_vibration[FlightPhase(phase)]=vibration(it.value());}
    p.validate();
}
json sensorInfo(const SensorProfile& p) {return {{"name",p.name},{"provenance",p.provenance},{"rate_hz",p.clock.rate_hz},{"internal_rate_hz",p.internal_rate_hz},{"nyquist_hz",p.clock.rate_hz/2},{"density",arr(p.stochastic.white_density)},{"full_scale",arr(p.full_scale)}};}
FlightFixture source(const json& request) {
    if(request.at("source")=="fixture")return verticalSixDofFixture();
    if(request.at("source")!="truth_csv")throw std::invalid_argument("unsupported flight truth source");
    std::ifstream in(request.at("truth_path").get<std::string>());std::string line;
    std::getline(in,line);if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(line!="time,x,y,z,vx,vy,vz,ax,ay,az,qw,qx,qy,qz,wx,wy,wz,gx,gy,gz,pressure_pa,temperature_k,phase")throw std::invalid_argument("truth CSV requires complete SI schema exported by EstimationValidation");
    FlightFixture fixture;fixture.source=request.at("truth_path").get<std::string>();
    while(std::getline(in,line)) {
        if(line.empty())continue;std::stringstream ss(line);std::string cell;std::vector<double> v;
        while(std::getline(ss,cell,',')){std::size_t used;double x=std::stod(cell,&used);if(used!=cell.size()&&cell.substr(used)!="\r")throw std::invalid_argument("invalid truth number");v.push_back(x);}
        if(v.size()!=23)throw std::invalid_argument("truth CSV column count");
        for(double x:v)if(!std::isfinite(x))throw std::invalid_argument("nonfinite truth");
        GroundTruth t;t.time=v[0];t.position={v[1],v[2],v[3]};t.velocity={v[4],v[5],v[6]};t.acceleration={v[7],v[8],v[9]};
        t.body_to_inertial={v[10],v[11],v[12],v[13]};t.angular_rate_body={v[14],v[15],v[16]};t.gravity={v[17],v[18],v[19]};
        t.pressure_pa=v[20];t.temperature_k=v[21];if(v[22]<0||v[22]>7||std::floor(v[22])!=v[22])throw std::invalid_argument("invalid truth phase");t.phase=FlightPhase(int(v[22]));fixture.samples.push_back(t);
    }fixture.trajectory();return fixture;
}
void add(json& plots,const std::string& variable,const std::string& label,const std::string& unit,double time,double value) {
    auto& s=plots[variable][label];s["unit"]=unit;s["points"].push_back({{"x",time},{"y",value}});
}
}
int main(int argc,char** argv) {
    try {
        if(argc!=3)throw std::invalid_argument("usage: avionics_validate request.json output-directory");
        std::ifstream input(argv[1]);json request;input>>request;
        if(request.at("schema_version")!=1)throw std::invalid_argument("unsupported request schema");
        if(request.value("operation","")=="preview_psd") {
            std::ifstream csv(request.at("path").get<std::string>());const auto spectrum=importPsdCsv(csv,request.value("g_squared",false));
            json plots=json::object();for(const auto& p:spectrum)for(int axis=0;axis<3;++axis)add(plots,"PSD",std::string("Requested ")+"XYZ"[axis],"(m/s²)²/Hz",p.frequency_hz,p.density[axis]);
            std::filesystem::create_directories(argv[2]);std::ofstream out(std::filesystem::path(argv[2])/"avionics.json");
            out<<json{{"schema_version",1},{"results",json::array({{{"scenario","PSD preview"},{"plots",plots},{"metrics",json::object()},{"events",json::array()},{"warnings",json::array()},{"analysis",json::object()},{"artifact_directory",argv[2]}}})}}.dump();return 0;
        }
        auto fixture=source(request);auto truth=fixture.trajectory();
        for(const auto& t:fixture.samples) {
            if(t.body_to_inertial.norm()<1e-12 || (t.body_to_inertial.normalized()*Vec3::UnitZ()-Vec3::UnitZ()).norm()>1e-6)
                throw std::invalid_argument("Vertical Kalman requires body +Z aligned with inertial +Z; incompatible attitude motion");
            if((t.gravity-Vec3(0,0,-g0)).norm()>1e-6)throw std::invalid_argument("reference estimator requires configured constant vertical gravity");
        }
        if(std::abs(fixture.samples.front().position.z())>1e-6 || std::abs(fixture.samples.front().velocity.z())>1e-6) throw std::invalid_argument("Reference initial prior requires zero altitude and vertical velocity");
        auto scenarios=loadScenarios(request.at("scenario_path"));json output={{"schema_version",1},{"results",json::array()},{"request",request}};
        const auto selected=request.at("scenarios").get<std::vector<std::string>>();if(selected.empty()||selected.size()>5)throw std::invalid_argument("select 1 to 5 scenarios");
        const auto controls=request.value("sensors",json::object());
        if(!controls.value("accel_enabled",true)||!controls.value("baro_enabled",true))throw std::invalid_argument("Vertical Kalman requires accelerometer and barometer enabled");
        std::filesystem::create_directories(argv[2]);
        for(const auto& name:selected) {
            auto it=std::find_if(scenarios.begin(),scenarios.end(),[&](const auto& s){return s.name==name;});if(it==scenarios.end())throw std::invalid_argument("unknown scenario");auto scenario=*it;
            auto& c=scenario.sensors;c.seed=std::stoull(request.at("seed").get<std::string>());c.diagnostics=request.value("diagnostics",true);
            sensor(c.accelerometer,controls.value("accel",json::object()),false);sensor(c.gyroscope,controls.value("gyro",json::object()),true);
            if(!controls.value("gyro_enabled",true))c.gyroscope.clock.dropout_probability=1; // generation excluded from delivered stream
            if(controls.contains("baro")) {
                auto j=controls.at("baro");auto profile=j.value("profile","SCENARIO");
                if(profile=="BMP280")c.barometer=bmp280(j.value("oversampling",16),j.value("iir",16),j.value("rate_hz",25.));
                else if(profile=="IDEAL")c.barometer=BarometerProfile{};
                else if(profile!="SCENARIO"&&profile!="CUSTOM")throw std::invalid_argument("unsupported barometer profile");
                auto& p=*c.barometer;if(j.contains("clock"))clockConfig(p.clock,j.at("clock"));p.noise_rms_pa=j.value("noise_rms_pa",p.noise_rms_pa);p.bias_pa=j.value("bias_pa",p.bias_pa);p.iir_coefficient=j.value("iir",p.iir_coefficient);
            }
            if(!controls.value("gnss_enabled",true))c.gnss.reset();
            else if(controls.contains("gnss")){auto j=controls.at("gnss");if(j.contains("clock"))clockConfig(c.gnss->clock,j.at("clock"));if(j.contains("position_rms"))c.gnss->position_rms_m=vec(j.at("position_rms"));if(j.contains("velocity_rms"))c.gnss->velocity_rms_mps=vec(j.at("velocity_rms"));}
            auto realization=simulate(truth,c);estimation::InitialState initial;initial.timestamp=truth.begin();
            auto results=compare(truth,realization.measurements,{{"Vertical Kalman",[&]{return std::make_unique<estimation::VerticalKalman>(scenario.estimator);}}},initial);
            const auto& result=results[0];auto directory=std::filesystem::path(argv[2])/name;exportDataset(directory.string(),fixture,scenario,realization,result);
            // The legacy exporter describes fixed regression defaults. Replace desktop metadata
            // with this request's actual provenance, retaining its units/counts/estimator contract.
            { json metadata; std::ifstream metaIn(directory/"metadata.json"); metaIn>>metadata; metaIn.close();
              metadata.erase("fixed_sensor_configuration"); metadata.erase("sensor_profiles"); metadata.erase("fixture");
              metadata["sensor_configuration"]=request; metadata["scenario_base_configuration"]=scenario.resolved_config;
              metadata["accel"]=sensorInfo(c.accelerometer); metadata["gyro"]=sensorInfo(c.gyroscope);
              std::ofstream metaOut(directory/"metadata.json"); metaOut<<metadata.dump(2); }
            json plots=json::object(),events=json::array(),warnings=json::array();
            for(const auto& t:fixture.samples){add(plots,"Altitude","Truth altitude","m",t.time,t.position.z());add(plots,"Vertical velocity","Truth velocity","m/s",t.time,t.velocity.z());add(plots,"Pressure","Truth pressure","Pa",t.time,t.pressure_pa);add(plots,"Accelerometer Z","Ideal specific force","m/s²",t.time,(t.body_to_inertial.conjugate()*(t.acceleration-t.gravity)).z());}
            std::size_t clipped=0,accels=0,boost=0,boost_clipped=0;
            for(const auto& m:realization.measurements)std::visit([&](const auto& s){
                using T=std::decay_t<decltype(s)>;const double t=s.stamp.truth_timestamp;
                if constexpr(std::is_same_v<T,PressureSample>){add(plots,"Pressure","Measured pressure","Pa",t,s.pressure_pa);add(plots,"Altitude","Barometric equivalent","m",t,estimation::pressureAltitude(s.pressure_pa,scenario.estimator).altitude_m);}
                else if constexpr(std::is_same_v<T,VectorSample>){if(m.kind==SensorKind::Accelerometer){add(plots,"Accelerometer Z","Measured accelerometer","m/s²",t,s.value.z());++accels;clipped+=s.saturated.z();if(truth.at(t).phase==FlightPhase::POWERED_ASCENT){++boost;boost_clipped+=s.saturated.z();}}else add(plots,"Gyroscope Z","Measured gyro (ignored)","rad/s",t,s.value.z());}
                else {add(plots,"GNSS Z","Measured position (ignored)","m",t,s.position_m.z());}
            },m.sample);
            for(const auto& row:result.history){const auto& s=row.state;add(plots,"Altitude","Estimated altitude","m",s.timestamp,s.altitude_m);add(plots,"Vertical velocity","Estimated velocity","m/s",s.timestamp,s.vertical_velocity_mps);add(plots,"Accel bias","Estimated bias","m/s²",s.timestamp,*s.accel_bias_mps2);add(plots,"Altitude error","Altitude error","m",s.timestamp,row.altitude_error_m);add(plots,"Velocity error","Velocity error","m/s",s.timestamp,row.velocity_error_mps);add(plots,"NEES","NEES [h,v]","1",s.timestamp,row.nees_hv);}
            for(const auto& i:result.innovations){add(plots,"Innovation","Pressure innovation","m",i.timestamp,i.residual_m);add(plots,"NIS","NIS","1",i.timestamp,i.nis);}
            for(std::size_t i=1;i<fixture.samples.size();++i){const auto& t=fixture.samples[i];if(t.phase!=fixture.samples[i-1].phase&&(t.phase==FlightPhase::IGNITION||t.phase==FlightPhase::BURNOUT||t.phase==FlightPhase::APOGEE_SEPARATION))events.push_back({{"enabled",true},{"x",t.time},{"label",t.phase==FlightPhase::IGNITION?"Ignition":t.phase==FlightPhase::BURNOUT?"Burnout":"Apogee"}});}
            std::vector<double> measured,generated,filtered;
            std::vector<std::pair<double,double>> orderedAccel;
            for(const auto& m:realization.measurements)if(m.kind==SensorKind::Accelerometer){const auto& a=std::get<VectorSample>(m.sample);orderedAccel.emplace_back(a.stamp.truth_timestamp,a.value.z());}
            std::sort(orderedAccel.begin(),orderedAccel.end());for(const auto& a:orderedAccel)measured.push_back(a.second);
            for(const auto& row:realization.diagnostics)if(row.kind==SensorKind::Accelerometer){const auto& d=row.channels;generated.push_back(d.vibration.z());filtered.push_back(d.post_filter.z());
                const std::vector<std::pair<std::string,Vec3>> parts={{"Ideal",d.ideal},{"Vibration",d.vibration},{"Bias",d.bias},{"Bias drift",d.bias_drift},{"White noise",d.white_noise},{"Scale error",d.scale_error},{"Misalignment",d.misalignment_error},{"Pre-filter",d.pre_filter},{"Post-filter",d.post_filter},{"Post-digital-filter",d.post_digital_filter},{"Pre-clip",d.pre_clip},{"Post-clip",d.post_clip},{"Quantized",d.quantized_output}};
                for(const auto& part:parts)add(plots,"Diagnostics",part.first,"m/s²",row.stamp.truth_timestamp,part.second.z());}
            json analysis={{"odr_hz",c.accelerometer.clock.rate_hz},{"internal_rate_hz",c.accelerometer.internal_rate_hz},{"nyquist_hz",c.accelerometer.clock.rate_hz/2},{"diagnostic_grid","ODR samples; high-frequency physical spectra cannot be reconstructed from these channels"}};
            auto psd=[&](const std::vector<double>& data,const std::string& label){if(data.size()<8)return;std::size_t n=std::min<std::size_t>(256,data.size()/2*2);auto w=sensors::validation::welch(data,c.accelerometer.clock.rate_hz,n);for(std::size_t k=0;k<w.density.size();++k)add(plots,"PSD",label,"(m/s²)²/Hz",w.frequency_hz[k],w.density[k]);if(label=="Measured PSD"){const auto peak=std::max_element(w.density.begin(),w.density.end())-w.density.begin();analysis["dominant_hz"]=w.frequency_hz[peak];analysis["integrated_psd"]=w.integrated_power;double power=0;for(double v:data)power+=v*v;analysis["rms"]=std::sqrt(power/data.size());}};
            if(c.accelerometer.clock.dropout_probability==0)psd(measured,"Measured PSD");
            else warnings.push_back("Measured PSD unavailable: dropped samples do not form a uniform ODR grid.");psd(generated,"Generated vibration PSD (ODR)");psd(filtered,"Post-filter PSD (ODR)");
            auto inspect=[&](const VibrationProfile& p){for(const auto& point:p.psd)add(plots,"PSD","Requested Z PSD","(m/s²)²/Hz",point.frequency_hz,point.density.z());for(const auto& h:p.harmonics)if(h.frequency_hz>c.accelerometer.clock.rate_hz/2){analysis["physical_hz"]=h.frequency_hz;analysis["expected_alias_hz"]=std::abs(std::remainder(h.frequency_hz,c.accelerometer.clock.rate_hz));warnings.push_back("Configured vibration exceeds output Nyquist; inspect aliasing.");}};
            inspect(c.accelerometer.vibration);for(const auto& p:c.accelerometer.phase_vibration)inspect(p.second);
            if(clipped)warnings.push_back("Accelerometer clipping: "+std::to_string(clipped)+" / "+std::to_string(accels)+" samples; powered ascent "+std::to_string(boost?100.*boost_clipped/boost:0)+"%.");
            warnings.push_back("Vertical Kalman assumes body +Z aligned with inertial +Z; gyro/GNSS are not used.");
            if(controls.contains("accel")||controls.contains("gyro")||controls.contains("baro"))warnings.push_back("Profile overrides are custom behavior; datasheet fidelity applies only to unmodified preset settings.");
            if(c.accelerometer.provenance.find("DATASHEET")!=std::string::npos||c.gyroscope.provenance.find("DATASHEET")!=std::string::npos)warnings.push_back("Datasheet typical: behavioral approximation, not a hardware digital twin.");
            json item={{"scenario",name},{"metrics",metricsJson(result.metrics)},{"plots",plots},{"events",events},{"warnings",warnings},{"analysis",analysis},{"accel",sensorInfo(c.accelerometer)},{"gyro",sensorInfo(c.gyroscope)},{"artifact_directory",directory.string()}};
            if(request.value("monte_carlo",false)){const auto trials=request.at("trials").get<std::size_t>();if(trials<1||trials>10000)throw std::invalid_argument("trials must be 1..10000");item["monte_carlo"]=monteCarloJson(monteCarlo(truth,scenario,c.seed,trials));}
            std::ofstream metrics(directory/"metrics.json");metrics<<item["metrics"].dump(2);
            std::ofstream configuration(directory/"configuration.json");configuration<<request.dump(2);
            std::ofstream mc(directory/"monte_carlo.json");mc<<item.value("monte_carlo",json::object()).dump(2);
            output["results"].push_back(std::move(item));std::cout<<"completed "<<name<<std::endl;
        }
        output["duration_s"]=truth.end()-truth.begin();std::ofstream out(std::filesystem::path(argv[2])/"avionics.json");out.exceptions(std::ios::failbit|std::ios::badbit);out<<output.dump();
    }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}return 0;
}
