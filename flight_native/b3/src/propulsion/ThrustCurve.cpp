#include "propulsion/ThrustCurve.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

std::string stripComment(const std::string& line) {
    const std::size_t semicolon = line.find(';');
    if (semicolon == std::string::npos) {
        return line;
    }
    return line.substr(0, semicolon);
}

bool isBlank(const std::string& line) {
    return std::all_of(line.begin(), line.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
}

std::vector<double> parseStandardDelays(const std::string& encoded) {
    std::vector<double> result;
    std::string normalized = encoded;
    std::replace(normalized.begin(), normalized.end(), ',', '-');
    std::istringstream stream(normalized);
    std::string token;
    while (std::getline(stream, token, '-')) {
        if (token.empty() || token == "P" || token == "p") {
            continue;
        }
        try {
            const double value = std::stod(token);
            if (value >= 0.0) {
                result.push_back(value);
            }
        } catch (const std::exception&) {
            // ENG permits plugged/non-numeric delay tokens; retain the raw
            // field while exposing only numeric delays in standardDelaysS.
        }
    }
    return result;
}

std::filesystem::path resolveEngPath(const std::string& path) {
    std::filesystem::path requested(path);
    if (std::filesystem::exists(requested)) {
        return requested;
    }

    if (requested.is_relative()) {
        std::filesystem::path motorsPath = std::filesystem::path("motors") / requested;
        if (std::filesystem::exists(motorsPath)) {
            return motorsPath;
        }
    }

    return requested;
}

} // namespace

ThrustCurve ThrustCurve::loadEngFile(const std::string& path) {
    const std::filesystem::path resolvedPath = resolveEngPath(path);
    std::ifstream input(resolvedPath);
    if (!input) {
        throw std::runtime_error("Could not open ENG motor file: " + path);
    }

    ThrustCurve curve;
    std::string line;
    bool headerParsed = false;

    while (std::getline(input, line)) {
        line = stripComment(line);
        if (isBlank(line)) {
            continue;
        }

        std::istringstream stream(line);
        if (!headerParsed) {
            if (!(stream >> curve.motorInfo.designation
                         >> curve.motorInfo.diameterMm
                         >> curve.motorInfo.lengthMm
                         >> curve.motorInfo.delays
                         >> curve.motorInfo.propellantMass
                         >> curve.motorInfo.totalMass)) {
                throw std::runtime_error("Invalid ENG header in file: " + resolvedPath.string());
            }

            std::getline(stream >> std::ws, curve.motorInfo.manufacturer);
            curve.motorInfo.standardDelaysS = parseStandardDelays(
                curve.motorInfo.delays);
            curve.motorInfo.dataSource = resolvedPath.string();
            headerParsed = true;
            continue;
        }

        double t = 0.0;
        double f = 0.0;
        if (!(stream >> t >> f)) {
            throw std::runtime_error("Invalid ENG time-thrust row in file: " + resolvedPath.string());
        }
        curve.time.push_back(t);
        curve.thrust.push_back(f);
    }

    curve.validate();
    curve.precompute();
    return curve;
}

double ThrustCurve::getThrust(double t) const {
    ensurePrecomputed();

    if (t < time.front() || t > time.back()) {
        return 0.0;
    }
    if (t == time.front()) {
        return thrust.front();
    }
    if (t == time.back()) {
        return thrust.back();
    }

    auto upper = std::upper_bound(time.begin(), time.end(), t);
    const std::size_t i = static_cast<std::size_t>(std::distance(time.begin(), upper));

    const double t0 = time[i - 1];
    const double f0 = thrust[i - 1];
    return f0 + segmentSlope_[i - 1] * (t - t0);
}

double ThrustCurve::impulseUntil(double t) const {
    ensurePrecomputed();

    if (t <= time.front()) {
        return 0.0;
    }
    if (t >= time.back()) {
        return cumulativeImpulse_.back();
    }

    auto upper = std::upper_bound(time.begin(), time.end(), t);
    const std::size_t i = static_cast<std::size_t>(std::distance(time.begin(), upper));

    const double t0 = time[i - 1];
    const double dt = t - t0;
    const double f0 = thrust[i - 1];
    const double fAtT = f0 + segmentSlope_[i - 1] * dt;

    return cumulativeImpulse_[i - 1] + 0.5 * (f0 + fAtT) * dt;
}

double ThrustCurve::impulseBetween(double t0, double t1) const {
    if (t1 < t0) {
        std::swap(t0, t1);
    }
    return impulseUntil(t1) - impulseUntil(t0);
}

double ThrustCurve::burnStartTime() const {
    validate();
    return time.front();
}

double ThrustCurve::burnEndTime() const {
    validate();
    return time.back();
}

double ThrustCurve::totalImpulse() const {
    ensurePrecomputed();

    return cumulativeImpulse_.back();
}

void ThrustCurve::precompute() const {
    validate();

    segmentSlope_.assign(time.size() - 1, 0.0);
    cumulativeImpulse_.assign(time.size(), 0.0);

    for (std::size_t i = 1; i < time.size(); ++i) {
        const double dt = time[i] - time[i - 1];
        segmentSlope_[i - 1] = (thrust[i] - thrust[i - 1]) / dt;
        cumulativeImpulse_[i] = cumulativeImpulse_[i - 1] + 0.5 * (thrust[i - 1] + thrust[i]) * dt;
    }
}

void ThrustCurve::ensurePrecomputed() const {
    if (segmentSlope_.size() != time.size() - 1 || cumulativeImpulse_.size() != time.size()) {
        precompute();
    }
}

void ThrustCurve::validate() const {
    if (time.size() != thrust.size() || time.size() < 2) {
        throw std::runtime_error("ThrustCurve requires at least two matching time-thrust samples.");
    }

    for (std::size_t i = 1; i < time.size(); ++i) {
        if (time[i] <= time[i - 1]) {
            throw std::runtime_error("ThrustCurve time values must be strictly increasing.");
        }
    }
}
