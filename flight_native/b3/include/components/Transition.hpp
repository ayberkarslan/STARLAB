#pragma once

#include <functional>
#include <string>

#include "components/Component.hpp"
#include "malzeme/Malzeme.h"

class Transition : public Component {
public:
    Transition(
        std::string name,
        double foreDiameter,
        double aftDiameter,
        double length,
        double thickness,
        const Malzeme& material
    );

    double foreDiameter() const noexcept;
    double aftDiameter() const noexcept;
    double foreRadius() const noexcept;
    double aftRadius() const noexcept;
    double length() const noexcept;
    double thickness() const noexcept;

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
    double cgZ() const;
    double cpZ() const;
    double cna(double referenceRadius) const;

    void printInfo() const;
    DragBreakdown dragBreakdown(const FlightCondition& fc) const override;

protected:
    MassProperties rawMassProperties() const override;
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;

private:
    double foreDiameter_ = 0.0;
    double aftDiameter_ = 0.0;
    double length_ = 0.0;
    double thickness_ = 0.0;
    Malzeme material_;

    double outerRadiusAt(double z) const;
    double innerRadiusAt(double z) const;
    double innerForeRadius() const;
    double innerAftRadius() const;
    double solidFrustumVolume(double r1, double r2) const;
    double solidFrustumCgZ(double r1, double r2) const;
    double referenceRadiusFrom(const FlightCondition& fc) const;
    double integrate(const std::function<double(double)>& f, int slices = 2000) const;
};
