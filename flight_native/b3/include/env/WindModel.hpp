#pragma once

#include "env/PinkNoise.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace env {

enum class WindShearModel {
    None,
    PowerLaw,
    Logarithmic,
};

struct WindConfig {
    double meanSpeedMs = 0.0;
    double fromDirectionDeg = 0.0;
    double directionGradientDegPerKm = 0.0;
    double launchAzimuthDeg = 0.0;
    WindShearModel shearModel = WindShearModel::PowerLaw;
    double referenceHeightM = 10.0;
    double minimumEffectiveHeightM = 1.0;
    double shearExponent = 0.14;
    double roughnessLengthM = 0.03;
    bool turbulenceEnabled = false;
    double turbulenceRmsMs = 0.0;
    double turbulenceAlpha = 1.0;
    int turbulencePoles = 5;
    double turbulenceSampleHz = 20.0;
    std::uint32_t turbulenceSeed = 1u;
};

struct WindSample {
    double eastMs = 0.0;
    double northMs = 0.0;
    double alongRangeMs = 0.0;
    double verticalMs = 0.0;
    double meanEastMs = 0.0;
    double meanNorthMs = 0.0;
    double meanAlongRangeMs = 0.0;
    double turbulenceEastMs = 0.0;
    double turbulenceNorthMs = 0.0;
    double turbulenceAlongRangeMs = 0.0;
    double directionFromDeg = 0.0;
    double directionToDeg = 180.0;
};

struct WindStep {
    double t0 = 0.0;
    double t1 = 0.0;
    double begin = 0.0;
    double end = 0.0;
    double turbulence0 = 0.0;
    double turbulence1 = 0.0;

    double sampleAt(double t) const noexcept;
};

class WindModel {
public:
    explicit WindModel(const WindConfig& config);

    WindStep prepareStep(double t, double dt);
    WindSample sample(const WindStep& step, double t, double altitudeAglM) const;
    const WindConfig& config() const noexcept { return config_; }

    static WindShearModel parseShearModel(const std::string& value);
    static const char* shearModelName(WindShearModel model) noexcept;
    static double normalizeDirectionDeg(double value) noexcept;
    static WindSample meteorologicalComponents(
        double speedMs, double fromDirectionDeg) noexcept;

private:
    double noiseAt(double t);
    double profileMultiplier(double altitudeAglM) const;

    WindConfig config_;
    PinkNoise pinkNoise_;
    double samplePeriodS_ = 0.05;
    std::vector<double> noiseSamples_;
};

} // namespace env
