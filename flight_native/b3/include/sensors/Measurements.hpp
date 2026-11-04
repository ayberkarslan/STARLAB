#pragma once
#include <Eigen/Core>
#include <cstdint>
#include <variant>
#include <vector>

// Estimators include this header only. No truth or diagnostic channels are exposed.
namespace sensors {
using Vec3 = Eigen::Vector3d;
struct Stamp {
    double truth_timestamp = 0;       // acquisition epoch, seconds
    double measurement_timestamp = 0; // delivery epoch: acquisition + latency + jitter
    std::uint64_t sequence = 0;       // gaps identify dropped acquisitions
};
struct VectorSample { Stamp stamp; Vec3 value = Vec3::Zero(); Eigen::Array<bool,3,1> saturated{false,false,false}; };
struct PressureSample { Stamp stamp; double pressure_pa = 0; };
struct GnssSample { Stamp stamp; Vec3 position_m = Vec3::Zero(), velocity_mps = Vec3::Zero(); };
enum class SensorKind { Accelerometer, Gyroscope, Barometer, Gnss };
struct Measurement { SensorKind kind; std::variant<VectorSample, PressureSample, GnssSample> sample; };
using MeasurementStream = std::vector<Measurement>;
class Estimator {
public:
    virtual ~Estimator() = default;
    virtual void consume(const Measurement&) = 0;
};
}
