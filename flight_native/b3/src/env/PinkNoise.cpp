#include "env/PinkNoise.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace env {

PinkNoise::PinkNoise(double alpha, int poles, std::uint32_t seed)
    : alpha_(alpha), poles_(poles), seed_(seed), engine_(seed) {
    if (!std::isfinite(alpha_) || alpha_ < 0.0 || alpha_ > 2.0) {
        throw std::invalid_argument("PinkNoise alpha must be between 0 and 2");
    }
    if (poles_ < 1 || poles_ > 16) {
        throw std::invalid_argument("PinkNoise poles must be between 1 and 16");
    }

    coefficients_.resize(static_cast<std::size_t>(poles_));
    history_.assign(static_cast<std::size_t>(poles_), 0.0);
    double coefficient = 1.0;
    for (int i = 0; i < poles_; ++i) {
        coefficient = (static_cast<double>(i) - alpha_ / 2.0)
            * coefficient / static_cast<double>(i + 1);
        coefficients_[static_cast<std::size_t>(i)] = coefficient;
    }
    rmsGain_ = calculateRmsGain();
    reset(seed_);
}

double PinkNoise::calculateRmsGain() const {
    // For white-noise input with unit variance, output variance equals the
    // sum of the squared impulse response.  This gives the public generator
    // an approximately unit-RMS output independent of alpha and pole count.
    std::vector<double> impulse(8192, 0.0);
    impulse[0] = 1.0;
    double energy = 1.0;
    for (std::size_t n = 1; n < impulse.size(); ++n) {
        double value = 0.0;
        const std::size_t count = std::min<std::size_t>(n, coefficients_.size());
        for (std::size_t k = 0; k < count; ++k) {
            value -= coefficients_[k] * impulse[n - k - 1];
        }
        impulse[n] = value;
        energy += value * value;
    }
    const double gain = std::sqrt(energy);
    return std::isfinite(gain) && gain > 1.0e-12 ? gain : 1.0;
}

void PinkNoise::reset(std::uint32_t seed) {
    seed_ = seed;
    engine_.seed(seed_);
    normal_.reset();
    std::fill(history_.begin(), history_.end(), 0.0);
    for (int i = 0; i < 20 * poles_; ++i) {
        (void)nextRawSample();
    }
}

double PinkNoise::nextRawSample() {
    double value = normal_(engine_);
    for (std::size_t i = 0; i < coefficients_.size(); ++i) {
        value -= coefficients_[i] * history_[i];
    }
    if (history_.size() > 1) {
        std::copy_backward(history_.begin(), history_.end() - 1, history_.end());
    }
    history_[0] = value;
    return value;
}

double PinkNoise::nextUnitSample() {
    return nextRawSample() / rmsGain_;
}

} // namespace env
