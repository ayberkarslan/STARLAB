#include "sensors/SyntheticSensors.hpp"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace sensors {
namespace {
void check(bool ok,const char* msg) { if(!ok) throw std::invalid_argument(msg); }
int choice(double x,std::initializer_list<double> values) {
    int i=0; for(double v:values) { if(x==v) return i; ++i; } throw std::invalid_argument("unsupported datasheet setting");
}
SensorProfile typical(const char* name,double rate,double range,double cutoff,Vec3 density) {
    SensorProfile p; p.name=name; p.provenance="DATASHEET_TYPICAL; matched-pole bandwidth approximation; output-equivalent white noise";
    p.clock.rate_hz=rate; p.internal_rate_hz=rate*std::ceil(std::max(4000.0,rate)/rate);
    p.full_scale=Vec3::Constant(range); p.saturation_enabled=true; p.setResolution(16);
    p.stochastic.white_enabled=true; p.stochastic.white_density=density;
    p.transfer.cutoff_hz=cutoff; p.validate(); return p;
}
std::vector<std::vector<double>> csv(std::istream& in,const std::string& header,std::size_t columns) {
    std::string line; check(bool(std::getline(in,line)),"empty CSV"); if(!line.empty()&&line.back()=='\r') line.pop_back();
    check(line==header,"unexpected CSV header/units"); std::vector<std::vector<double>> rows;
    while(std::getline(in,line)) {
        if(!line.empty()&&line.back()=='\r') line.pop_back();
        if(line.empty()) continue;
        std::stringstream stream(line); std::string cell; std::vector<double> row;
        while(std::getline(stream,cell,',')) { std::size_t used=0; double x=std::stod(cell,&used); check(used==cell.size()&&std::isfinite(x),"invalid CSV number"); row.push_back(x); }
        check(row.size()==columns&&line.back()!=',',"invalid CSV column count"); rows.push_back(std::move(row));
    }
    check(rows.size()>=2,"CSV requires at least two rows"); return rows;
}
Vec3 vector(const nlohmann::json& j) {
    check(j.is_array()&&j.size()==3,"profile vector must have three axes"); Vec3 v;
    for(int i=0;i<3;++i) v[i]=j.at(i).get<double>();
    check(v.allFinite(),"nonfinite profile vector"); return v;
}
}
SensorProfile bmi088Accelerometer(double odr,double range,unsigned osr) {
    const int row=choice(odr,{12.5,25,50,100,200,400,800,1600}); choice(range,{3,6,12,24}); const int col=choice(osr,{1,2,4});
    const double bw[8][3]={{5,2,1},{10,5,3},{20,9,5},{40,19,10},{80,38,20},{145,75,40},{230,140,80},{280,234,145}};
    // Rev 1.9: 160 ug/sqrt(Hz) X/Y, 190 Z (normal mode, +/-3g).
    auto p=typical("BMI088 accelerometer",odr,range*g0,bw[row][col],Vec3(160e-6,160e-6,190e-6)*g0);
    p.cross_axis_tolerance_fraction=0.005;
    if((row==6&&col==0)||(row==7&&col<2)) p.provenance+="; shared XY cutoff used, Z bandwidth differs in hardware";
    return p;
}
SensorProfile bmi088Gyroscope(unsigned setting,double range) {
    check(setting<8,"BMI088 gyro bandwidth register must be 0..7"); choice(range,{125,250,500,1000,2000});
    const double rates[]={2000,2000,1000,400,200,100,200,100},bw[]={523,230,116,47,23,12,64,32};
    auto p=typical("BMI088 gyroscope",rates[setting],range*pi/180,bw[setting],Vec3::Constant(0.014*pi/180));
    p.sensitivity_tolerance_fraction=0.01; p.cross_axis_tolerance_fraction=0.01;
    return p;
}
SensorProfile mpu6500Accelerometer(double odr,double range,double cutoff) {
    choice(range,{2,4,8,16}); choice(cutoff,{5.05,10.2,21.2,44.8,99,218.1,420});
    check(std::isfinite(odr)&&odr>0&&odr<=1000&&std::abs(1000/odr-std::round(1000/odr))<1e-9&&1000/odr<=256,"MPU filtered ODR must be 1000/(1+divider), divider 0..255");
    auto p=typical("MPU-6500 accelerometer",odr,range*g0,cutoff,Vec3::Constant(300e-6*g0));
    p.sensitivity_tolerance_fraction=0.03;
    return p;
}
SensorProfile mpu6500Gyroscope(double odr,double range,double cutoff) {
    choice(range,{250,500,1000,2000}); choice(cutoff,{5,10,20,41,92,184});
    check(std::isfinite(odr)&&odr>0&&odr<=1000&&std::abs(1000/odr-std::round(1000/odr))<1e-9&&1000/odr<=256,"MPU filtered ODR must be 1000/(1+divider), divider 0..255");
    auto p=typical("MPU-6500 gyroscope",odr,range*pi/180,cutoff,Vec3::Constant(0.01*pi/180));
    p.sensitivity_tolerance_fraction=0.03;
    return p;
}
BarometerProfile bmp280(unsigned os,unsigned coefficient,double odr) {
    const int r=choice(os,{1,2,4,8,16}),c=choice(coefficient,{1,2,4,8,16});
    const double rms[5][5]={{3.3,1.9,1.2,0.9,0.4},{2.6,1.5,1.0,0.6,0.4},{2.1,1.2,0.8,0.5,0.3},{1.6,1.0,0.6,0.4,0.2},{1.3,0.8,0.5,0.4,0.2}};
    // Conservative max conversion time, temperature oversampling x1, pressure xOS.
    check(std::isfinite(odr)&&odr>0&&odr<=1000/(1.25+2.3+2.3*os+0.575),"BMP280 ODR exceeds conversion budget");
    BarometerProfile p; p.name="BMP280"; p.provenance="DATASHEET_TYPICAL; table 8 output RMS matched using effective white drive";
    p.clock.rate_hz=odr; p.iir_coefficient=coefficient; p.datasheet_output_rms_pa=rms[r][c];
    p.noise_rms_pa=rms[r][c]*std::sqrt(2.0*coefficient-1); p.lsb_pa=0.16; return p;
}
SensorProfile importCharacterizedProfile(std::istream& in) {
    nlohmann::json j; in>>j;
    check(j.is_object(),"profile must be an object");
    const std::vector<std::string> allowed={"schema_version","kind","name","source","units","rate_hz","internal_rate_hz","white_density","random_walk","gm_sigma","gm_tau_s"};
    for(auto it=j.begin();it!=j.end();++it) check(std::find(allowed.begin(),allowed.end(),it.key())!=allowed.end(),"unknown profile field");
    check(j.at("schema_version")==1,"unsupported characterization schema");
    const std::string kind=j.at("kind"),units=j.at("units");
    check((kind=="accelerometer"&&units=="m/s^2")||(kind=="gyroscope"&&units=="rad/s"),"profile kind/units mismatch");
    SensorProfile p; p.name=j.at("name").get<std::string>(); p.provenance="CHARACTERIZED: "+j.at("source").get<std::string>();
    p.clock.rate_hz=j.at("rate_hz").get<double>(); p.internal_rate_hz=j.at("internal_rate_hz").get<double>();
    if(j.contains("white_density")) {p.stochastic.white_density=vector(j.at("white_density"));p.stochastic.white_enabled=true;}
    if(j.contains("random_walk")) {p.stochastic.random_walk=vector(j.at("random_walk"));p.stochastic.random_walk_enabled=true;}
    check(j.contains("gm_sigma")==j.contains("gm_tau_s"),"GM requires sigma and tau together");
    if(j.contains("gm_sigma")) {p.stochastic.gm_sigma=vector(j.at("gm_sigma"));p.stochastic.gm_tau_s=vector(j.at("gm_tau_s"));p.stochastic.gm_enabled=true;}
    p.validate(); return p;
}
std::vector<PsdPoint> importPsdCsv(std::istream& in,bool g_squared) {
    auto rows=csv(in,"frequency_Hz,PSD_X,PSD_Y,PSD_Z",4); std::vector<PsdPoint> p; double last=-1;
    for(const auto& r:rows) { check(r[0]>=0&&r[0]>last&&r[1]>=0&&r[2]>=0&&r[3]>=0,"invalid PSD frequencies/densities"); p.push_back({r[0],Vec3(r[1],r[2],r[3])*(g_squared?g0*g0:1)}); last=r[0]; } return p;
}
RecordedImu importRecordedCsv(std::istream& in,bool dc,bool detrend) {
    auto rows=csv(in,"time,ax,ay,az,gx,gy,gz",7); RecordedImu out; double last=-std::numeric_limits<double>::infinity(); const double start=rows.front()[0];
    for(const auto& r:rows) {check(r[0]>last,"recorded timestamps must increase"); out.accel.push_back({r[0]-start,Vec3(r[1],r[2],r[3])}); out.gyro.push_back({r[0]-start,Vec3(r[4],r[5],r[6])}); last=r[0];}
    auto clean=[&](std::vector<RecordedPoint>& p) {
        Vec3 mean=Vec3::Zero(),slope=Vec3::Zero(); double mt=0,tt=0;
        for(const auto& r:p) {mean+=r.value;mt+=r.time;} mean/=p.size();mt/=p.size();
        if(detrend) {for(const auto& r:p) {tt+=(r.time-mt)*(r.time-mt);slope+=(r.time-mt)*(r.value-mean);} slope/=tt;}
        for(auto& r:p) {if(dc||detrend) r.value-=mean; if(detrend) r.value-=slope*(r.time-mt);}
    }; clean(out.accel);clean(out.gyro); return out;
}
}
