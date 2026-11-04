#pragma once

#include <string>

#include "components/Component.hpp"
#include "malzeme/Malzeme.h"

class RailButton : public Component {
public:
    RailButton(std::string name, double outerDiameter, double innerDiameter,
               double height, double baseHeight, double flangeHeight,
               const Malzeme& material);

    ComponentTrait traits() const noexcept override {
        return ComponentTrait::ExternalComponent | ComponentTrait::LeafComponent;
    }
    double axialLength() const override;
    double outerDiameter() const noexcept { return outerDiameter_; }
    double innerDiameter() const noexcept { return innerDiameter_; }
    double height() const noexcept { return height_; }

protected:
    MassProperties rawMassProperties() const override;

private:
    double outerDiameter_ = 0.0;
    double innerDiameter_ = 0.0;
    double height_ = 0.0;
    double baseHeight_ = 0.0;
    double flangeHeight_ = 0.0;
    Malzeme material_;
};
