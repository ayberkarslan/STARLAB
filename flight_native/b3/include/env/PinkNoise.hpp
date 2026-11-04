#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace env {

class PinkNoise {
public:
    PinkNoise(double alpha = 1.0, int poles = 5, std::uint32_t seed = 1u);

    double nextUnitSample();
    void reset(std::uint32_t seed);

    double alpha() const noexcept { return alpha_; }
    int poles() const noexcept { return poles_; }
    std::uint32_t seed() const noexcept { return seed_; }

private:
    double nextRawSample();
    double calculateRmsGain() const;

    double alpha_;
    int poles_;
    std::uint32_t seed_;
    double rmsGain_ = 1.0;
    std::vector<double> coefficients_;
    std::vector<double> history_;
    std::mt19937 engine_;
    std::normal_distribution<double> normal_{0.0, 1.0};
};

} // namespace env
