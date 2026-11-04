#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <Eigen/Dense>

#include "components/Component.hpp"
#include "malzeme/Malzeme.h"

class FinSet : public Component {
public:
    FinSet(std::string name, int n_fins, double thickness, const Malzeme& material, std::string cross_section = "square");
    virtual ~FinSet() = default;

    int numFins() const noexcept { return n_fins_; }
    double thickness() const noexcept { return thickness_; }
    std::string crossSection() const noexcept { return cross_section_; }

    double kSection() const;
    double surfaceDensity() const;

    // Must return the (z, r) coordinates of the fin polygon
    virtual std::vector<Eigen::Vector2d> planformPoints() const = 0;

    double axialLength() const override;
    ComponentTrait traits() const noexcept override {
        return ComponentCategory::ExternalComponent | ComponentCategory::LeafComponent;
    }

    double planformArea() const;
    Eigen::Vector2d centroid() const;
    DragBreakdown dragBreakdown(const FlightCondition& fc) const override;

    // Returns parent's reference radius. Defaults to 0.0 if no parent.
    double getReferenceOuterRadius() const override;

protected:
    MassProperties rawMassProperties() const override;
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;

private:
    int n_fins_;
    double thickness_;
    Malzeme material_;
    std::string cross_section_;

    std::vector<Eigen::Vector2d> closedPoints() const;
    double polygonSignedArea() const;
    std::tuple<double, double, double> areaSecondMomentsAboutOrigin() const;
    std::tuple<double, double, double> areaSecondMomentsAboutCentroid() const;

    double singleFinMass() const;
    Eigen::Matrix3d singleFinInertiaLocal() const;
};

class TrapezoidalFinSet : public FinSet {
public:
    TrapezoidalFinSet(
        std::string name,
        int n_fins,
        double thickness,
        const Malzeme& material,
        double root_chord,
        double tip_chord,
        double span,
        double sweep_length,
        std::string cross_section = "square"
    );

    std::vector<Eigen::Vector2d> planformPoints() const override;

protected:
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;

private:
    double root_chord_;
    double tip_chord_;
    double span_;
    double sweep_length_;
};

class EllipticalFinSet : public FinSet {
public:
    EllipticalFinSet(
        std::string name,
        int n_fins,
        double thickness,
        const Malzeme& material,
        double root_chord,
        double span,
        std::string cross_section = "square"
    );
    EllipticalFinSet(
        std::string name,
        int n_fins,
        double thickness,
        const Malzeme& material,
        double root_chord,
        double tip_chord,
        double span,
        double sweep_length,
        std::string cross_section = "square"
    );

    std::vector<Eigen::Vector2d> planformPoints() const override;

protected:
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;

private:
    double root_chord_;
    double span_;
};
