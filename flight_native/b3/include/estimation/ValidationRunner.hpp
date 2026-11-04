#pragma once
#include "estimation/StateEstimator.hpp"
#include "estimation/VerticalKalman.hpp"
#include "sensors/SyntheticSensors.hpp"
#include "sensors/Validation.hpp"
#include "nlohmann/json.hpp"
#include <functional>
#include <memory>
#include <string>

namespace estimation::validation {
struct FlightFixture {
    std::vector<sensors::GroundTruth> samples;
    std::string source;
    double ignition_s=1, burnout_s=6;
    sensors::TruthTrajectory trajectory() const { return sensors::TruthTrajectory(samples); }
};
FlightFixture verticalSixDofFixture();
struct Scenario {
    std::string name, description;
    sensors::RunConfig sensors;
    VerticalConfig estimator;
    nlohmann::json resolved_config;
};
std::vector<Scenario> loadScenarios(const std::string& path);
struct ConsistencySummary {
    std::size_t count=0;
    unsigned degrees_of_freedom=0;
    double mean=0, inside_fraction=0, lower=0, upper=0;
};
ConsistencySummary consistency(const std::vector<double>& values, unsigned dof);
struct Metrics {
    sensors::validation::ErrorMetrics altitude{}, velocity{};
    double apogee_altitude_error_m=0;
    std::optional<double> apogee_time_error_s;
    ConsistencySummary nis, nees_hv;
};
struct EstimateRecord {
    double delivery_timestamp;
    EstimatedState state;
    double altitude_error_m=0, velocity_error_mps=0, nees_hv=0;
};
struct EstimatorResult {
    std::string name;
    std::vector<EstimateRecord> history;
    std::vector<Innovation> innovations;
    Metrics metrics;
};
using Factory=std::function<std::unique_ptr<StateEstimator>()>;
// Generates no sensors: every estimator receives the same const measurement objects.
std::vector<EstimatorResult> compare(const sensors::TruthTrajectory& truth,
    const sensors::MeasurementStream& measurements,
    const std::vector<std::pair<std::string,Factory>>& estimators,
    const InitialState& initial = {});
struct Aggregate { double mean=0,median=0,standard_deviation=0,p95=0,worst=0; };
Aggregate aggregate(const std::vector<double>& values);
struct MonteCarloResult {
    std::vector<std::uint64_t> seeds;
    std::vector<Metrics> trials;
    Aggregate altitude_rmse,velocity_rmse,altitude_max,velocity_max,apogee_absolute_error,mean_nis,nis_inside_fraction;
};
MonteCarloResult monteCarlo(const sensors::TruthTrajectory&,const Scenario&,std::uint64_t master,std::size_t trials);
nlohmann::json metricsJson(const Metrics&);
nlohmann::json monteCarloJson(const MonteCarloResult&);
void exportDataset(const std::string& directory,const FlightFixture&,const Scenario&,
                   const sensors::SyntheticSensorRun&,const EstimatorResult&);
}
