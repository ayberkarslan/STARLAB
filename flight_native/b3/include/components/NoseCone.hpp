#pragma once

#include <array>
#include <functional>
#include <string>

#include "malzeme/Malzeme.h"
#include "components/Component.hpp"

class NoseCone : public Component {
public:
    NoseCone(
        std::string name,
        double baseDiameter,
        double length,
        const Malzeme& material,
        double thickness = 0.0,
        std::string shape = "conical",
        double shoulderLength = 0.0,
        double shoulderOuterDiameter = 0.0,
        double shoulderThickness = 0.0
    );

    double baseDiameter() const noexcept;
    double baseRadius() const noexcept;
    double length() const noexcept;
    double thickness() const noexcept;
    std::string shape() const noexcept;
    double shoulderLength() const noexcept;
    double shoulderOuterDiameter() const noexcept;
    double shoulderOuterRadius() const noexcept;
    double shoulderThickness() const noexcept;

    double axialLength() const override;
    ComponentTrait traits() const noexcept override {
        return ComponentTrait::BodyComponent | ComponentTrait::ExternalComponent;
    }
    bool isCompatible(const Component& child) const noexcept override {
        return child.hasAnyTrait(
            ComponentCategory::InternalComponent | ComponentCategory::FreeformFinSet);
    }
    double getReferenceOuterRadius() const override;

    double volume() const;
    void printInfo() const;
    DragBreakdown dragBreakdown(const FlightCondition& fc) const override;

protected:
    MassProperties rawMassProperties() const override;
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;

private:
    double baseDiameter_ = 0.0;
    double length_ = 0.0;
    double thickness_ = 0.0;
    std::string shape_ = "conical";
    double shoulderLength_ = 0.0;
    double shoulderOuterDiameter_ = 0.0;
    double shoulderThickness_ = 0.0;
    Malzeme material_;

    double shoulderInnerRadius() const;
    double radiusAt(double z) const;
    double innerRadiusAt(double z) const;
    double noseCgZ() const;
    double noseIzzCg() const;
    double noseIxxCg() const;
    double shoulderCgZ() const;
    double shoulderMass() const;
    double shoulderIzzCg() const;
    double shoulderIxxCg() const;
    double trapezoidIntegrate(const std::function<double(double)>& f, int points = 2000) const;
};
