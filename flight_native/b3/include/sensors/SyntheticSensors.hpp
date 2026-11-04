#pragma once
#include "sensors/Measurements.hpp"
#include <Eigen/Geometry>
#include <functional>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <istream>

namespace sensors {
constexpr double pi = 3.14159265358979323846;
constexpr double g0 = 9.80665;
enum class FlightPhase { PRELAUNCH, IGNITION, POWERED_ASCENT, BURNOUT, COAST, APOGEE_SEPARATION, PARACHUTE_DEPLOYMENT, DESCENT };
struct GroundTruth {
    double time = 0;
    Vec3 position = Vec3::Zero(), velocity = Vec3::Zero(), acceleration = Vec3::Zero();
    // Hamilton scalar-first quaternion: ACTIVE body -> right-handed inertial frame.
    Eigen::Quaterniond body_to_inertial = Eigen::Quaterniond::Identity();
    Vec3 angular_rate_body = Vec3::Zero(), gravity = Vec3(0,0,-g0);
    double pressure_pa = 101325, temperature_k = 288.15;
    std::optional<Vec3> magnetic_field_tesla;
    FlightPhase phase = FlightPhase::PRELAUNCH;
};
class TruthTrajectory {
    std::vector<GroundTruth> states_;
public:
    explicit TruthTrajectory(std::vector<GroundTruth> states);
    GroundTruth at(double time) const; // linear vectors, shortest-arc quaternion SLERP; no extrapolation
    double begin() const { return states_.front().time; }
    double end() const { return states_.back().time; }
};
struct SensorClock {
    double rate_hz = 100, latency_s = 0, jitter_std_s = 0, dropout_probability = 0;
    void validate() const;
    double time(double start, std::uint64_t k) const { return start + double(k)/rate_hz; }
};
std::uint64_t deriveSeed(std::uint64_t master, std::uint64_t stream);
class RandomSource {
    std::mt19937_64 engine_;
public:
    explicit RandomSource(std::uint64_t seed): engine_(seed) {}
    double uniform();
    double normal(); // specified Box-Muller, no global or standard-library normal distribution
    Vec3 normal3();
};
struct StochasticConfig {
    Vec3 white_density = Vec3::Zero(); // measurement units * sqrt(s), Kalibr convention
    Vec3 random_walk = Vec3::Zero();   // measurement units / sqrt(s)
    Vec3 gm_sigma = Vec3::Zero(), gm_tau_s = Vec3::Ones();
    bool white_enabled = false, random_walk_enabled = false, gm_enabled = false;
    void validate() const;
};
class StochasticProcess {
    StochasticConfig config_;
    RandomSource white_, walk_, gm_;
    Vec3 walk_state_ = Vec3::Zero(), gm_state_ = Vec3::Zero();
public:
    StochasticProcess(StochasticConfig config, std::uint64_t seed);
    Vec3 advance(double dt); // bias at t=0 is zero; GM converges from zero
    Vec3 white(double dt);
};
struct Calibration {
    bool bias_enabled = false, scale_enabled = false, misalignment_enabled = false;
    Vec3 bias = Vec3::Zero(), scale_error = Vec3::Zero();
    Eigen::Matrix3d misalignment = Eigen::Matrix3d::Identity();
};
enum class VibrationMode { OFF, RANDOM_RMS, SINUSOIDAL, PSD, RECORDED };
struct Harmonic { double frequency_hz = 0; Vec3 amplitude = Vec3::Zero(), phase_rad = Vec3::Zero(); };
struct PsdPoint { double frequency_hz; Vec3 density; }; // one-sided SI units squared / Hz
struct RecordedPoint { double time; Vec3 value; };
struct VibrationProfile {
    VibrationMode mode = VibrationMode::OFF;
    Vec3 rms = Vec3::Zero();
    Eigen::Matrix3d correlation = Eigen::Matrix3d::Identity();
    std::vector<Harmonic> harmonics;
    std::vector<PsdPoint> psd;
    std::vector<RecordedPoint> recorded;
};
class VibrationModel {
    VibrationProfile profile_;
    RandomSource rng_;
    Eigen::Matrix3d factor_ = Eigen::Matrix3d::Identity();
    std::vector<Vec3> realization_;
    double rate_;
public:
    VibrationModel(VibrationProfile profile, double internal_rate, double duration, std::uint64_t seed);
    Vec3 at(double elapsed);
    Vec3 expectedVariance() const;
};
enum class AntialiasMode { REAL_SENSOR_FILTER, NO_ANTIALIAS_TEST_MODE };
struct TransferConfig {
    AntialiasMode mode = AntialiasMode::REAL_SENSOR_FILTER;
    double cutoff_hz = 0; // zero bypass; repeated matched-pole first-order sections
    unsigned order = 1;
};
class SensorTransferFunction {
    double alpha_;
    std::vector<Vec3> states_;
    bool initialized_ = false;
public:
    SensorTransferFunction(TransferConfig config, double rate);
    Vec3 step(Vec3 input);
};
struct SensorProfile {
    std::string name = "CUSTOM", provenance = "USER_CONFIGURED";
    // Unsigned datasheet bounds are metadata, not invented signed calibration values.
    std::optional<double> sensitivity_tolerance_fraction, cross_axis_tolerance_fraction;
    SensorClock clock;
    double internal_rate_hz = 1000;
    Calibration calibration;
    StochasticConfig stochastic;
    TransferConfig transfer;
    TransferConfig digital_transfer; // optional output-rate LPF, before clipping/quantization
    VibrationProfile vibration;
    std::map<FlightPhase,VibrationProfile> phase_vibration;
    bool saturation_enabled = false, quantization_enabled = false;
    Vec3 full_scale = Vec3::Ones(), lsb = Vec3::Zero();
    void validate() const;
    void setResolution(unsigned bits);
};
struct SensorDiagnostics {
    Vec3 ideal, bias, bias_drift, white_noise, vibration, scale_error, misalignment_error;
    Vec3 pre_filter, post_filter, pre_clip, post_clip, quantized_output;
    Vec3 pre_digital_filter, post_digital_filter;
};
struct DiagnosticSample { SensorKind kind; Stamp stamp; SensorDiagnostics channels; };
struct PressureDiagnostics { Stamp stamp; double ideal, bias, white_noise, pre_filter, post_filter, quantized_output; };
struct GnssDiagnostics { Stamp stamp; Vec3 ideal_position, ideal_velocity, position_bias, velocity_bias, position_noise, velocity_noise; };
struct BarometerProfile {
    std::string name = "CUSTOM", provenance = "USER_CONFIGURED";
    SensorClock clock;
    double bias_pa = 0, noise_rms_pa = 0, lsb_pa = 0;
    unsigned iir_coefficient = 1; // y += (x-y)/coefficient; 1 bypass
    double datasheet_output_rms_pa = 0; // metadata; input RMS is set to reproduce this after IIR
};
struct GnssProfile {
    SensorClock clock{10,0,0,0};
    Vec3 position_bias_m = Vec3::Zero(), velocity_bias_mps = Vec3::Zero();
    Vec3 position_rms_m = Vec3::Zero(), velocity_rms_mps = Vec3::Zero();
};
struct RunConfig {
    SensorProfile accelerometer, gyroscope;
    std::optional<BarometerProfile> barometer;
    std::optional<GnssProfile> gnss;
    bool diagnostics = false;
    std::uint64_t seed = 0;
};
struct SyntheticSensorRun {
    MeasurementStream measurements;
    std::vector<DiagnosticSample> diagnostics;
    std::vector<PressureDiagnostics> pressure_diagnostics;
    std::vector<GnssDiagnostics> gnss_diagnostics;
};
SyntheticSensorRun simulate(const TruthTrajectory&, const RunConfig&);
// Imports fail on malformed/unknown data. Fitted coefficients only; no runtime Allan estimator.
SensorProfile importCharacterizedProfile(std::istream& json);
std::vector<PsdPoint> importPsdCsv(std::istream&, bool acceleration_g_squared = false);
struct RecordedImu { std::vector<RecordedPoint> accel, gyro; };
RecordedImu importRecordedCsv(std::istream&, bool remove_dc = true, bool detrend = false);
SensorProfile bmi088Accelerometer(double odr = 100, double range_g = 6, unsigned osr = 4);
SensorProfile bmi088Gyroscope(unsigned bandwidth_setting = 7, double range_dps = 500);
SensorProfile mpu6500Accelerometer(double odr = 1000, double range_g = 16, double cutoff = 99);
SensorProfile mpu6500Gyroscope(double odr = 1000, double range_dps = 2000, double cutoff = 92);
BarometerProfile bmp280(unsigned pressure_oversampling = 16, unsigned iir_coefficient = 16, double odr = 25);
}
