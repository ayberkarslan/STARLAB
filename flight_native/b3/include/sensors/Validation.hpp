#pragma once
#include "sensors/SyntheticSensors.hpp"
#include <Eigen/Dense>
namespace sensors::validation {
struct WelchResult { std::vector<double> frequency_hz, density; double integrated_power=0; };
WelchResult welch(const std::vector<double>& signal,double rate,std::size_t segment_length);
struct PsdConsistency { double measured_mean_square, welch_power, target_power, relative_energy_error; bool consistent; };
PsdConsistency checkPsd(const std::vector<double>& signal,double rate,std::size_t segment_length,double target_power,double relative_tolerance=0.15);
struct ErrorMetrics { double rmse,mae,mean,standard_deviation,max_absolute,p50_absolute,p95_absolute,p99_absolute; };
ErrorMetrics errors(const std::vector<double>& signed_errors);
double attitudeError(const Eigen::Quaterniond& truth,const Eigen::Quaterniond& estimate);
// NIS for innovations, NEES for state errors; validation-only full covariance API.
double normalizedSquaredError(const Eigen::VectorXd& error,const Eigen::MatrixXd& covariance);
}
