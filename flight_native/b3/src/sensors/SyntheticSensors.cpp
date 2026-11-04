#include "sensors/SyntheticSensors.hpp"
#include <Eigen/Cholesky>
#include <unsupported/Eigen/FFT>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace sensors {
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::invalid_argument(message); }
bool nonnegative(const Vec3& x) { return x.allFinite() && (x.array()>=0).all(); }
bool positive(double x) { return std::isfinite(x) && x>0; }
Vec3 psdAt(const std::vector<PsdPoint>& p, double f) {
    if(f<p.front().frequency_hz || f>p.back().frequency_hz) return Vec3::Zero();
    auto upper=std::upper_bound(p.begin(),p.end(),f,[](double v,const PsdPoint& a){return v<a.frequency_hz;});
    if(upper==p.end()) return p.back().density;
    const auto& a=*(upper-1); const auto& b=*upper;
    return a.density+(b.density-a.density)*((f-a.frequency_hz)/(b.frequency_hz-a.frequency_hz));
}
std::optional<Stamp> stamp(const SensorClock& c, double time, std::uint64_t k, RandomSource& dropout, RandomSource& jitter) {
    const double j=c.jitter_std_s>0 ? c.jitter_std_s*jitter.normal() : 0;
    if(c.dropout_probability>0 && dropout.uniform()<c.dropout_probability) return {};
    return Stamp{time,time+c.latency_s+j,k};
}
}

TruthTrajectory::TruthTrajectory(std::vector<GroundTruth> states): states_(std::move(states)) {
    require(!states_.empty(),"empty truth trajectory");
    double last=-std::numeric_limits<double>::infinity();
    for(auto& s:states_) {
        require(std::isfinite(s.time)&&s.time>last,"truth times must be finite and strictly increasing");
        require(s.position.allFinite()&&s.velocity.allFinite()&&s.acceleration.allFinite()&&s.gravity.allFinite()&&s.angular_rate_body.allFinite(),"nonfinite truth vector");
        require(s.body_to_inertial.coeffs().allFinite()&&s.body_to_inertial.norm()>1e-12,"invalid truth quaternion");
        require(positive(s.pressure_pa)&&positive(s.temperature_k),"invalid atmosphere truth");
        if(s.magnetic_field_tesla) require(s.magnetic_field_tesla->allFinite(),"invalid magnetic field");
        s.body_to_inertial.normalize(); last=s.time;
    }
}
GroundTruth TruthTrajectory::at(double t) const {
    if(!std::isfinite(t)||t<begin()-1e-10||t>end()+1e-10) throw std::out_of_range("sensor time outside truth trajectory");
    if(t<=begin()) return states_.front();
    if(t>=end()) return states_.back();
    auto it=std::upper_bound(states_.begin(),states_.end(),t,[](double x,const GroundTruth& s){return x<s.time;});
    const auto& b=*it; const auto& a=*(it-1); const double u=(t-a.time)/(b.time-a.time);
    GroundTruth s=a; s.time=t;
    s.position=a.position+u*(b.position-a.position); s.velocity=a.velocity+u*(b.velocity-a.velocity);
    s.acceleration=a.acceleration+u*(b.acceleration-a.acceleration); s.gravity=a.gravity+u*(b.gravity-a.gravity);
    s.angular_rate_body=a.angular_rate_body+u*(b.angular_rate_body-a.angular_rate_body);
    s.body_to_inertial=a.body_to_inertial.slerp(u,b.body_to_inertial);
    s.pressure_pa=a.pressure_pa+u*(b.pressure_pa-a.pressure_pa); s.temperature_k=a.temperature_k+u*(b.temperature_k-a.temperature_k);
    if(a.magnetic_field_tesla&&b.magnetic_field_tesla) s.magnetic_field_tesla=*a.magnetic_field_tesla+u*(*b.magnetic_field_tesla-*a.magnetic_field_tesla);
    else s.magnetic_field_tesla.reset();
    return s;
}
void SensorClock::validate() const {
    require(positive(rate_hz)&&std::isfinite(latency_s)&&latency_s>=0&&std::isfinite(jitter_std_s)&&jitter_std_s>=0,"invalid sensor timing");
    require(std::isfinite(dropout_probability)&&dropout_probability>=0&&dropout_probability<=1,"dropout must be in [0,1]");
}
std::uint64_t deriveSeed(std::uint64_t master,std::uint64_t stream) {
    std::uint64_t x=master+0x9e3779b97f4a7c15ULL*(stream+1);
    x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL; x=(x^(x>>27))*0x94d049bb133111ebULL;
    return x^(x>>31);
}
double RandomSource::uniform() { return (double(engine_()>>11)+0.5)*0x1.0p-53; }
double RandomSource::normal() { double u=uniform(),v=uniform(); return std::sqrt(-2*std::log(u))*std::cos(2*pi*v); }
Vec3 RandomSource::normal3() { Vec3 v; for(int i=0;i<3;++i) v[i]=normal(); return v; }
void StochasticConfig::validate() const {
    require(nonnegative(white_density)&&nonnegative(random_walk)&&nonnegative(gm_sigma),"invalid stochastic amplitudes");
    require(gm_tau_s.allFinite()&&(gm_tau_s.array()>0).all(),"GM correlation time must be positive");
}
StochasticProcess::StochasticProcess(StochasticConfig c,std::uint64_t seed):config_(c),white_(deriveSeed(seed,0)),walk_(deriveSeed(seed,1)),gm_(deriveSeed(seed,2)) { c.validate(); }
Vec3 StochasticProcess::advance(double dt) {
    require(std::isfinite(dt)&&dt>=0,"invalid stochastic step");
    if(dt==0) return walk_state_+gm_state_;
    if(config_.random_walk_enabled) walk_state_+=config_.random_walk.cwiseProduct(walk_.normal3())*std::sqrt(dt);
    if(config_.gm_enabled) {
        Vec3 n=gm_.normal3();
        for(int i=0;i<3;++i) { double phi=std::exp(-dt/config_.gm_tau_s[i]); gm_state_[i]=phi*gm_state_[i]+config_.gm_sigma[i]*std::sqrt(-std::expm1(-2*dt/config_.gm_tau_s[i]))*n[i]; }
    }
    return walk_state_+gm_state_;
}
Vec3 StochasticProcess::white(double dt) {
    require(positive(dt),"invalid white-noise period");
    if(!config_.white_enabled) return Vec3::Zero();
    return config_.white_density.cwiseProduct(white_.normal3())/std::sqrt(dt);
}
VibrationModel::VibrationModel(VibrationProfile p,double rate,double duration,std::uint64_t seed):profile_(std::move(p)),rng_(seed),rate_(rate) {
    require(positive(rate)&&std::isfinite(duration)&&duration>=0,"invalid vibration grid");
    require(nonnegative(profile_.rms),"invalid vibration RMS");
    const auto& c=profile_.correlation;
    require(c.allFinite()&&c.isApprox(c.transpose(),1e-12)&&c.diagonal().isApprox(Vec3::Ones(),1e-12),"invalid axis correlation");
    Eigen::LLT<Eigen::Matrix3d> llt(c); require(llt.info()==Eigen::Success,"correlation must be positive definite"); factor_=llt.matrixL();
    for(const auto& h:profile_.harmonics) require(std::isfinite(h.frequency_hz)&&h.frequency_hz>=0&&h.frequency_hz<rate/2&&h.amplitude.allFinite()&&h.phase_rad.allFinite(),"harmonic must lie below internal Nyquist");
    if(profile_.mode==VibrationMode::RECORDED) {
        require(profile_.recorded.size()>=2,"recorded vibration needs two samples");
        double last=-1;
        for(const auto& r:profile_.recorded) {
            require(std::isfinite(r.time)&&r.time>last&&r.value.allFinite(),"invalid recorded vibration");
            if(last>=0) require((r.time-last)*rate>=1-1e-9,"internal rate undersamples recorded data");
            last=r.time;
        }
        require(profile_.recorded.front().time==0&&last>=duration,"recorded vibration must cover run starting at zero");
    }
    if(profile_.mode!=VibrationMode::PSD) return;
    require(profile_.psd.size()>=2,"PSD needs two frequency knots");
    double last=-1;
    for(const auto& r:profile_.psd) { require(std::isfinite(r.frequency_hz)&&r.frequency_hz>=0&&r.frequency_hz>last&&nonnegative(r.density),"invalid PSD knots"); last=r.frequency_hz; }
    require(last<rate/2,"PSD support must lie below internal Nyquist");
    std::size_t n=2; const double needed=std::ceil(duration*rate)+1;
    require(needed<=16777216,"PSD realization too large; segment the run");
    while(n<needed) n*=2;
    realization_.assign(n,Vec3::Zero());
    std::vector<std::vector<std::complex<double>>> spectra(3,std::vector<std::complex<double>>(n));
    const double df=rate/n;
    for(std::size_t k=1;k<n/2;++k) {
        Vec3 density=psdAt(profile_.psd,k*df);
        Vec3 re=factor_*rng_.normal3(),im=factor_*rng_.normal3();
        for(int axis=0;axis<3;++axis) {
            // Eigen inverse FFT divides by N. E[one-sided periodogram]=PSD.
            double scale=double(n)*std::sqrt(density[axis]*df/4);
            spectra[axis][k]=scale*std::complex<double>(re[axis],im[axis]);
            spectra[axis][n-k]=std::conj(spectra[axis][k]);
        }
    }
    Eigen::FFT<double> fft;
    for(int axis=0;axis<3;++axis) { std::vector<double> out; fft.inv(out,spectra[axis]); for(std::size_t k=0;k<n;++k) realization_[k][axis]=out[k]; }
}
Vec3 VibrationModel::expectedVariance() const {
    if(profile_.mode==VibrationMode::RANDOM_RMS) return profile_.rms.cwiseProduct(profile_.rms);
    Vec3 v=Vec3::Zero();
    if(profile_.mode==VibrationMode::PSD) for(std::size_t k=1;k<profile_.psd.size();++k) v+=(profile_.psd[k].density+profile_.psd[k-1].density)*0.5*(profile_.psd[k].frequency_hz-profile_.psd[k-1].frequency_hz);
    return v;
}
Vec3 VibrationModel::at(double t) {
    require(std::isfinite(t)&&t>=0,"invalid vibration time");
    switch(profile_.mode) {
    case VibrationMode::OFF: return Vec3::Zero();
    case VibrationMode::RANDOM_RMS: return profile_.rms.cwiseProduct(factor_*rng_.normal3());
    case VibrationMode::SINUSOIDAL: {
        Vec3 v=Vec3::Zero(); for(const auto& h:profile_.harmonics) v+=h.amplitude.cwiseProduct((h.phase_rad.array()+2*pi*h.frequency_hz*t).sin().matrix()); return v;
    }
    case VibrationMode::PSD: {
        auto k=static_cast<std::size_t>(std::llround(t*rate_));
        if(k>=realization_.size()) throw std::out_of_range("PSD time outside realization");
        return realization_[k];
    }
    case VibrationMode::RECORDED: {
        const auto& rows=profile_.recorded;
        if(t>rows.back().time+1e-10) throw std::out_of_range("recorded replay exhausted");
        auto it=std::upper_bound(rows.begin(),rows.end(),t,[](double x,const RecordedPoint& r){return x<r.time;});
        if(it==rows.begin()) return it->value;
        if(it==rows.end()) return rows.back().value;
        auto& a=*(it-1); return a.value+(it->value-a.value)*((t-a.time)/(it->time-a.time));
    }
    }
    throw std::invalid_argument("unknown vibration mode");
}
SensorTransferFunction::SensorTransferFunction(TransferConfig c,double rate):alpha_(1) {
    require(positive(rate)&&std::isfinite(c.cutoff_hz)&&c.cutoff_hz>=0&&c.cutoff_hz<rate/2&&c.order>=1&&c.order<=16,"invalid filter configuration");
    if(c.mode==AntialiasMode::REAL_SENSOR_FILTER&&c.cutoff_hz>0) { alpha_=-std::expm1(-2*pi*c.cutoff_hz/rate); states_.assign(c.order,Vec3::Zero()); }
}
Vec3 SensorTransferFunction::step(Vec3 x) {
    for(auto& s:states_) { if(!initialized_) s=x; else s+=alpha_*(x-s); x=s; } initialized_=true; return x;
}
void SensorProfile::setResolution(unsigned bits) {
    require(bits>=2&&bits<=32&&full_scale.allFinite()&&(full_scale.array()>0).all(),"invalid resolution/range");
    lsb=2*full_scale/std::ldexp(1.0,int(bits)); quantization_enabled=true;
}
void SensorProfile::validate() const {
    clock.validate(); stochastic.validate();
    require(positive(internal_rate_hz)&&internal_rate_hz>=clock.rate_hz,"internal rate must cover ODR");
    const double ratio=internal_rate_hz/clock.rate_hz;
    require(std::abs(ratio-std::round(ratio))<1e-9,"internal rate must be an integer multiple of ODR");
    require(calibration.bias.allFinite()&&calibration.scale_error.allFinite()&&calibration.misalignment.allFinite(),"invalid calibration");
    for(const auto& bound:{sensitivity_tolerance_fraction,cross_axis_tolerance_fraction})
        if(bound) require(std::isfinite(*bound)&&*bound>=0,"invalid datasheet tolerance");
    require(full_scale.allFinite()&&(full_scale.array()>0).all()&&nonnegative(lsb),"invalid range/LSB");
    if(quantization_enabled) require((lsb.array()>0).all()&&(!saturation_enabled||(lsb.array()<=full_scale.array()).all()),"quantization requires positive LSB within range");
    SensorTransferFunction check(transfer,internal_rate_hz);
    SensorTransferFunction digital_check(digital_transfer,clock.rate_hz);
}
namespace {
void vectorRun(const TruthTrajectory& truth,const SensorProfile& p,SensorKind kind,std::uint64_t seed,bool debug,SyntheticSensorRun& out) {
    p.validate(); const double duration=truth.end()-truth.begin();
    require(duration*p.internal_rate_hz<1e8,"sensor run too large");
    std::map<FlightPhase,VibrationModel> phases;
    for(const auto& kv:p.phase_vibration) phases.emplace(kv.first,VibrationModel(kv.second,p.internal_rate_hz,duration,deriveSeed(seed,100+int(kv.first))));
    VibrationModel vibration(p.vibration,p.internal_rate_hz,duration,deriveSeed(seed,3));
    StochasticProcess noise(p.stochastic,deriveSeed(seed,4));
    RandomSource dropout(deriveSeed(seed,5)),jitter(deriveSeed(seed,6));
    SensorTransferFunction filter(p.transfer,p.internal_rate_hz);
    SensorTransferFunction digital_filter(p.digital_transfer,p.clock.rate_hz);
    const auto stride=static_cast<std::uint64_t>(std::llround(p.internal_rate_hz/p.clock.rate_hz));
    const auto count=static_cast<std::uint64_t>(std::floor(duration*p.internal_rate_hz+1e-8));
    for(std::uint64_t k=0;k<=count;++k) {
        const double time=truth.begin()+double(k)/p.internal_rate_hz;
        const auto t=truth.at(time); SensorDiagnostics d;
        if(kind==SensorKind::Accelerometer) d.ideal=t.body_to_inertial.conjugate()*(t.acceleration-t.gravity);
        else d.ideal=t.angular_rate_body;
        auto phase=phases.find(t.phase);
        d.vibration=(phase==phases.end()?vibration:phase->second).at(double(k)/p.internal_rate_hz);
        d.pre_filter=d.ideal+d.vibration; d.post_filter=filter.step(d.pre_filter);
        // Bias/noise are output-equivalent sensor errors, updated at acquisition ODR.
        if(k%stride!=0) continue;
        const auto seq=k/stride;
        Vec3 x=d.post_filter;
        d.misalignment_error.setZero(); d.scale_error.setZero();
        if(p.calibration.misalignment_enabled) { Vec3 y=p.calibration.misalignment*x; d.misalignment_error=y-x; x=y; }
        if(p.calibration.scale_enabled) { d.scale_error=p.calibration.scale_error.cwiseProduct(x); x+=d.scale_error; }
        d.bias=p.calibration.bias_enabled?p.calibration.bias:Vec3::Zero();
        d.bias_drift=noise.advance(seq==0?0:1/p.clock.rate_hz); d.white_noise=noise.white(1/p.clock.rate_hz);
        d.pre_digital_filter=x+d.bias+d.bias_drift+d.white_noise;
        d.post_digital_filter=digital_filter.step(d.pre_digital_filter);
        d.pre_clip=d.post_digital_filter; d.post_clip=d.pre_clip;
        VectorSample sample;
        for(int axis=0;axis<3;++axis) {
            if(p.saturation_enabled) { sample.saturated[axis]=std::abs(d.pre_clip[axis])>p.full_scale[axis]; d.post_clip[axis]=std::clamp(d.pre_clip[axis],-p.full_scale[axis],p.full_scale[axis]); }
            d.quantized_output[axis]=d.post_clip[axis];
            if(p.quantization_enabled) {
                double code=std::round(d.post_clip[axis]/p.lsb[axis]);
                if(p.saturation_enabled) code=std::clamp(code,std::ceil(-p.full_scale[axis]/p.lsb[axis]),std::floor(p.full_scale[axis]/p.lsb[axis]));
                d.quantized_output[axis]=code*p.lsb[axis];
            }
        }
        auto st=stamp(p.clock,p.clock.time(truth.begin(),seq),seq,dropout,jitter);
        if(!st) continue;
        sample.stamp=*st; sample.value=d.quantized_output;
        out.measurements.push_back({kind,sample}); if(debug) out.diagnostics.push_back({kind,*st,d});
    }
}
}
SyntheticSensorRun simulate(const TruthTrajectory& truth,const RunConfig& config) {
    SyntheticSensorRun out;
    vectorRun(truth,config.accelerometer,SensorKind::Accelerometer,deriveSeed(config.seed,0),config.diagnostics,out);
    vectorRun(truth,config.gyroscope,SensorKind::Gyroscope,deriveSeed(config.seed,1),config.diagnostics,out);
    if(config.barometer) {
        const auto& p=*config.barometer; p.clock.validate();
        require(std::isfinite(p.bias_pa)&&std::isfinite(p.noise_rms_pa)&&p.noise_rms_pa>=0&&std::isfinite(p.lsb_pa)&&p.lsb_pa>=0&&p.iir_coefficient>=1,"invalid barometer profile");
        RandomSource rng(deriveSeed(config.seed,20)),dropout(deriveSeed(config.seed,21)),jitter(deriveSeed(config.seed,22));
        double filtered=0;
        require((truth.end()-truth.begin())*p.clock.rate_hz<1e8,"barometer run too large");
        for(std::uint64_t k=0;;++k) {
            const double time=p.clock.time(truth.begin(),k); if(time>truth.end()+1e-10) break;
            const double ideal=truth.at(time).pressure_pa,noise=p.noise_rms_pa*rng.normal();
            const double input=ideal+p.bias_pa+noise;
            if(k==0) filtered=input; else filtered+=(input-filtered)/p.iir_coefficient;
            const double output=p.lsb_pa>0?p.lsb_pa*std::round(filtered/p.lsb_pa):filtered;
            auto st=stamp(p.clock,time,k,dropout,jitter);
            if(st) {
                out.measurements.push_back({SensorKind::Barometer,PressureSample{*st,output}});
                if(config.diagnostics) out.pressure_diagnostics.push_back({*st,ideal,p.bias_pa,noise,input,filtered,output});
            }
        }
    }
    if(config.gnss) {
        const auto& p=*config.gnss; p.clock.validate();
        require(nonnegative(p.position_rms_m)&&nonnegative(p.velocity_rms_mps)&&p.position_bias_m.allFinite()&&p.velocity_bias_mps.allFinite(),"invalid GNSS profile");
        RandomSource pos(deriveSeed(config.seed,30)),vel(deriveSeed(config.seed,31)),dropout(deriveSeed(config.seed,32)),jitter(deriveSeed(config.seed,33));
        require((truth.end()-truth.begin())*p.clock.rate_hz<1e8,"GNSS run too large");
        for(std::uint64_t k=0;;++k) {
            const double time=p.clock.time(truth.begin(),k); if(time>truth.end()+1e-10) break;
            auto t=truth.at(time); GnssSample s;
            const Vec3 position_noise=p.position_rms_m.cwiseProduct(pos.normal3()),velocity_noise=p.velocity_rms_mps.cwiseProduct(vel.normal3());
            s.position_m=t.position+p.position_bias_m+position_noise;
            s.velocity_mps=t.velocity+p.velocity_bias_mps+velocity_noise;
            auto st=stamp(p.clock,time,k,dropout,jitter);
            if(st) {
                s.stamp=*st; out.measurements.push_back({SensorKind::Gnss,s});
                if(config.diagnostics) out.gnss_diagnostics.push_back({*st,t.position,t.velocity,p.position_bias_m,p.velocity_bias_mps,position_noise,velocity_noise});
            }
        }
    }
    std::stable_sort(out.measurements.begin(),out.measurements.end(),[](const Measurement& a,const Measurement& b){
        auto time=[](const Measurement& m){return std::visit([](const auto& s){return s.stamp.measurement_timestamp;},m.sample);}; return time(a)<time(b);
    });
    return out;
}
}
