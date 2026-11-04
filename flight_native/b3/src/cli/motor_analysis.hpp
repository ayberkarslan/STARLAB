#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace motor_analysis {

using json = nlohmann::json;
constexpr double g0 = 9.80665;

inline double trapezoid(const std::vector<double>& time,
                        const std::vector<double>& thrust) {
    if (time.size() != thrust.size() || time.size() < 2) {
        throw std::runtime_error(
            "Motor curve requires at least two matching time/thrust samples.");
    }
    double impulse = 0.0;
    for (std::size_t i = 1; i < time.size(); ++i) {
        const double dt = time[i] - time[i - 1];
        if (dt < 0.0) {
            throw std::runtime_error("Motor curve time values must be sorted.");
        }
        impulse += 0.5 * (thrust[i - 1] + thrust[i]) * dt;
    }
    return impulse;
}

inline json analyzeCurve(std::vector<double> time,
                         std::vector<double> thrust,
                         double requestedBurnTime = -1.0) {
    if (time.size() != thrust.size() || time.size() < 2) {
        throw std::runtime_error(
            "Motor curve requires at least two matching time/thrust samples.");
    }

    std::vector<std::pair<double, double>> points;
    points.reserve(time.size());
    for (std::size_t i = 0; i < time.size(); ++i) {
        points.emplace_back(time[i], std::max(0.0, thrust[i]));
    }
    std::stable_sort(points.begin(), points.end());
    time.clear();
    thrust.clear();
    for (const auto& point : points) {
        if (!time.empty() && std::abs(point.first - time.back()) < 1e-12) {
            thrust.back() = point.second;
        } else {
            time.push_back(point.first);
            thrust.push_back(point.second);
        }
    }
    if (time.size() < 2) {
        throw std::runtime_error("Motor curve needs two distinct time values.");
    }

    const double burnTime =
        requestedBurnTime > 0.0 ? requestedBurnTime : time.back();
    if (time.front() > 0.0) {
        time.insert(time.begin(), 0.0);
        thrust.insert(thrust.begin(), 0.0);
    }
    if (burnTime > time.back()) {
        time.push_back(burnTime);
        thrust.push_back(0.0);
    }

    const double impulse = trapezoid(time, thrust);
    const double average = burnTime > 0.0 ? impulse / burnTime : 0.0;
    const double maximum =
        *std::max_element(thrust.begin(), thrust.end());

    json curve = json::array();
    for (std::size_t i = 0; i < time.size(); ++i) {
        curve.push_back({time[i], thrust[i]});
    }
    return {
        {"time_s", time},
        {"thrust_n", thrust},
        {"curve_points", curve},
        {"burn_time_s", burnTime},
        {"total_impulse_ns", impulse},
        {"average_thrust_n", average},
        {"maximum_thrust_n", maximum},
    };
}

inline json generate(const json& request) {
    const std::string mode = request.value("mode", "generated");
    const double burnTime = request.value("burn_time_s", 2.0);
    if (!(burnTime > 0.0)) {
        throw std::runtime_error("Motor burn_time_s must be positive.");
    }

    if (mode == "manual") {
        return analyzeCurve(
            request.at("time_s").get<std::vector<double>>(),
            request.at("thrust_n").get<std::vector<double>>(),
            burnTime);
    }

    double average = request.value("average_thrust_n", -1.0);
    if (request.contains("propellant_mass_kg") && request.contains("isp_s")) {
        average = request.at("propellant_mass_kg").get<double>()
            * request.at("isp_s").get<double>() * g0 / burnTime;
    }
    if (!(average >= 0.0)) {
        throw std::runtime_error(
            "Motor generation needs average thrust or propellant mass and Isp.");
    }

    std::vector<double> relativeTime;
    std::vector<double> relativeThrust;
    if (request.value("profile", "wizard") == "catalog") {
        relativeTime = {0.0, 0.05, 0.15, 0.30, 0.50, 0.70, 0.85, 0.95, 1.0};
        relativeThrust = {0.0, 0.80, 1.30, 1.15, 1.05, 0.95, 0.85, 0.40, 0.0};
    } else {
        relativeTime = {0.0, 0.06, 0.16, 0.34, 0.58, 0.78, 0.92, 1.0};
        relativeThrust = {0.0, 0.85, 1.32, 1.12, 1.00, 0.82, 0.35, 0.0};
    }

    std::vector<double> time(relativeTime.size());
    std::vector<double> thrust(relativeThrust.size());
    for (std::size_t i = 0; i < relativeTime.size(); ++i) {
        time[i] = relativeTime[i] * burnTime;
        thrust[i] = relativeThrust[i] * average;
    }
    const double rawImpulse = trapezoid(time, thrust);
    const double targetImpulse = burnTime * average;
    const double scale = rawImpulse > 0.0 ? targetImpulse / rawImpulse : 1.0;
    for (double& value : thrust) {
        value *= scale;
    }
    return analyzeCurve(std::move(time), std::move(thrust), burnTime);
}

inline json process(const json& request) {
    if (request.value("operation", "generate") == "analyze") {
        return analyzeCurve(
            request.at("time_s").get<std::vector<double>>(),
            request.at("thrust_n").get<std::vector<double>>(),
            request.value("burn_time_s", -1.0));
    }
    return generate(request);
}

}  // namespace motor_analysis
