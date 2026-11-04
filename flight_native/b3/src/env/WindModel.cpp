#include "env/WindModel.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace env {
namespace {
constexpr double pi = 3.14159265358979323846;

double degreesToRadians(double value) {
    return value * pi / 180.0;
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
} // namespace

double WindStep::sampleAt(double t) const noexcept {
    const double duration = t1 - t0;
    double fraction = duration > 0.0
        ? std::clamp((t - t0) / duration, 0.0, 1.0) : 0.0;
    if (std::abs(fraction) < 1.0e-10) fraction = 0.0;
    if (std::abs(fraction - 1.0) < 1.0e-10) fraction = 1.0;
    return begin + fraction * (end - begin);
}

WindModel::WindModel(const WindConfig& config)
    : config_(config),
      pinkNoise_(config.turbulenceAlpha, config.turbulencePoles,
                 config.turbulenceSeed) {
    if (!std::isfinite(config_.meanSpeedMs) || config_.meanSpeedMs < 0.0) {
        throw std::invalid_argument("wind_speed_ms must be finite and non-negative");
    }
    if (!std::isfinite(config_.fromDirectionDeg)
        || !std::isfinite(config_.directionGradientDegPerKm)) {
        throw std::invalid_argument("wind direction and gradient must be finite");
    }
    if (!std::isfinite(config_.referenceHeightM) || config_.referenceHeightM <= 0.0) {
        throw std::invalid_argument("wind_reference_height_m must be positive");
    }
    if (!std::isfinite(config_.minimumEffectiveHeightM)
        || config_.minimumEffectiveHeightM < 0.0
        || config_.minimumEffectiveHeightM > config_.referenceHeightM) {
        throw std::invalid_argument(
            "wind_min_effective_height_m must be between zero and reference height");
    }
    if (!std::isfinite(config_.shearExponent) || config_.shearExponent < 0.0
        || config_.shearExponent > 1.0) {
        throw std::invalid_argument("wind_shear_exponent must be between 0 and 1");
    }
    if (!std::isfinite(config_.roughnessLengthM) || config_.roughnessLengthM <= 0.0
        || (config_.shearModel == WindShearModel::Logarithmic
            && config_.roughnessLengthM >= config_.referenceHeightM)) {
        throw std::invalid_argument("wind_roughness_length_m must be positive and below reference height");
    }
    if (!std::isfinite(config_.turbulenceRmsMs) || config_.turbulenceRmsMs < 0.0) {
        throw std::invalid_argument("turbulence_rms_ms must be finite and non-negative");
    }
    if (!std::isfinite(config_.turbulenceSampleHz)
        || config_.turbulenceSampleHz < 0.5 || config_.turbulenceSampleHz > 500.0) {
        throw std::invalid_argument("turbulence_sample_hz must be between 0.5 and 500");
    }
    samplePeriodS_ = 1.0 / config_.turbulenceSampleHz;
    noiseSamples_.push_back(config_.turbulenceEnabled
        ? pinkNoise_.nextUnitSample() : 0.0);
}

WindShearModel WindModel::parseShearModel(const std::string& value) {
    const std::string normalized = lowercase(value);
    if (normalized == "none" || normalized == "constant" || normalized == "yok (sabit)") {
        return WindShearModel::None;
    }
    if (normalized == "log" || normalized == "log_law"
        || normalized == "logarithmic"
        || normalized.find("logarit") != std::string::npos) {
        return WindShearModel::Logarithmic;
    }
    return WindShearModel::PowerLaw;
}

const char* WindModel::shearModelName(WindShearModel model) noexcept {
    switch (model) {
        case WindShearModel::None: return "none";
        case WindShearModel::Logarithmic: return "log_law";
        case WindShearModel::PowerLaw: return "power_law";
    }
    return "power_law";
}

double WindModel::normalizeDirectionDeg(double value) noexcept {
    double normalized = std::fmod(value, 360.0);
    if (normalized < 0.0) normalized += 360.0;
    return normalized;
}

WindSample WindModel::meteorologicalComponents(
    double speedMs, double fromDirectionDeg) noexcept {
    WindSample sample;
    sample.directionFromDeg = normalizeDirectionDeg(fromDirectionDeg);
    sample.directionToDeg = normalizeDirectionDeg(sample.directionFromDeg + 180.0);
    const double fromRadians = degreesToRadians(sample.directionFromDeg);
    // Meteorological direction says where the wind comes FROM.
    sample.eastMs = -speedMs * std::sin(fromRadians);
    sample.northMs = -speedMs * std::cos(fromRadians);
    return sample;
}

double WindModel::noiseAt(double t) {
    if (!config_.turbulenceEnabled || config_.turbulenceRmsMs <= 0.0) return 0.0;
    const double safeTime = std::max(t, 0.0);
    const double position = safeTime / samplePeriodS_;
    // Snap values that are only floating-point noise below an exact sample
    // boundary.  This keeps the absolute-time process independent of RK4 dt.
    const std::size_t index = static_cast<std::size_t>(
        std::floor(position + 1.0e-10));
    while (noiseSamples_.size() <= index + 1) {
        noiseSamples_.push_back(pinkNoise_.nextUnitSample());
    }
    const double fraction = std::clamp(
        position - static_cast<double>(index), 0.0, 1.0);
    return config_.turbulenceRmsMs
        * (noiseSamples_[index]
           + fraction * (noiseSamples_[index + 1] - noiseSamples_[index]));
}

WindStep WindModel::prepareStep(double t, double dt) {
    WindStep step;
    step.t0 = t;
    step.t1 = t + std::max(dt, 0.0);
    step.turbulence0 = noiseAt(step.t0);
    step.turbulence1 = noiseAt(step.t1);
    step.begin = step.turbulence0;
    step.end = step.turbulence1;
    return step;
}

double WindModel::profileMultiplier(double altitudeAglM) const {
    if (config_.shearModel == WindShearModel::None) return 1.0;
    double height = std::max(altitudeAglM, 0.0);
    if (config_.shearModel == WindShearModel::Logarithmic) {
        if (height <= config_.roughnessLengthM) return 0.0;
        const double numerator = std::log(height / config_.roughnessLengthM);
        const double denominator = std::log(
            config_.referenceHeightM / config_.roughnessLengthM);
        return std::max(numerator / denominator, 0.0);
    }
    height = std::max(height, config_.minimumEffectiveHeightM);
    if (height <= 0.0) return 0.0;
    return std::pow(height / config_.referenceHeightM, config_.shearExponent);
}

WindSample WindModel::sample(
    const WindStep& step, double t, double altitudeAglM) const {
    WindSample result;
    const double direction = normalizeDirectionDeg(
        config_.fromDirectionDeg
        + config_.directionGradientDegPerKm * std::max(altitudeAglM, 0.0) / 1000.0);
    const double meanSpeed = config_.meanSpeedMs * profileMultiplier(altitudeAglM);
    const WindSample mean = meteorologicalComponents(meanSpeed, direction);
    const double requestedPerturbation = step.sampleAt(t);
    // Turbulence changes only the speed magnitude. Clamp at zero so a large
    // negative perturbation cannot reverse the meteorological wind direction.
    const double instantaneousSpeed =
        std::max(meanSpeed + requestedPerturbation, 0.0);
    const WindSample instantaneous =
        meteorologicalComponents(instantaneousSpeed, direction);
    result.directionFromDeg = direction;
    result.directionToDeg = normalizeDirectionDeg(direction + 180.0);
    result.meanEastMs = mean.eastMs;
    result.meanNorthMs = mean.northMs;
    result.eastMs = instantaneous.eastMs;
    result.northMs = instantaneous.northMs;
    result.turbulenceEastMs = result.eastMs - result.meanEastMs;
    result.turbulenceNorthMs = result.northMs - result.meanNorthMs;
    const double launchAzimuth = degreesToRadians(config_.launchAzimuthDeg);
    const double launchEast = std::sin(launchAzimuth);
    const double launchNorth = std::cos(launchAzimuth);
    result.meanAlongRangeMs = result.meanEastMs * launchEast
        + result.meanNorthMs * launchNorth;
    result.turbulenceAlongRangeMs = result.turbulenceEastMs * launchEast
        + result.turbulenceNorthMs * launchNorth;
    result.alongRangeMs = result.meanAlongRangeMs
        + result.turbulenceAlongRangeMs;
    return result;
}

} // namespace env
