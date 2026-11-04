#include "components/RailButton.hpp"

#include <cmath>
#include <stdexcept>

RailButton::RailButton(std::string name, double outerDiameter, double innerDiameter,
                       double height, double baseHeight, double flangeHeight,
                       const Malzeme& material)
    : outerDiameter_(outerDiameter), innerDiameter_(innerDiameter), height_(height),
      baseHeight_(baseHeight), flangeHeight_(flangeHeight), material_(material) {
    name_ = std::move(name);
    if (outerDiameter_ <= 0.0 || innerDiameter_ < 0.0 || innerDiameter_ > outerDiameter_
        || height_ <= 0.0 || baseHeight_ < 0.0 || flangeHeight_ < 0.0) {
        throw std::invalid_argument("Invalid RailButton dimensions.");
    }
}

double RailButton::axialLength() const { return height_; }

MassProperties RailButton::rawMassProperties() const {
    const double ro = outerDiameter_ * 0.5;
    const double ri = innerDiameter_ * 0.5;
    const double axialThickness = std::max(baseHeight_ + flangeHeight_, 1e-6);
    const double volume = std::acos(-1.0) * (ro * ro - ri * ri) * axialThickness;
    MassProperties result;
    result.mass = material_.alYogunluk() * volume;
    result.cgLocal = Eigen::Vector3d(0.0, 0.0, height_ * 0.5);
    result.inertiaLocal(2, 2) = 0.5 * result.mass * (ro * ro + ri * ri);
    result.inertiaLocal(0, 0) = result.inertiaLocal(1, 1)
        = result.mass * (3.0 * (ro * ro + ri * ri) + height_ * height_) / 12.0;
    return result;
}
