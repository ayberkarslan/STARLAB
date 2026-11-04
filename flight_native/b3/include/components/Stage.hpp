#pragma once

#include <limits>
#include <string>

#include "aero/DragBreakdown.hpp"
#include "components/Component.hpp"

class Stage : public Component {
public:
    explicit Stage(std::string name);

    double axialLength() const override;
    ComponentTrait traits() const noexcept override {
        return ComponentCategory::ComponentAssembly | ComponentCategory::AxialStage;
    }
    bool isCompatible(const Component& child) const noexcept override {
        return child.hasAnyTrait(ComponentCategory::BodyComponent);
    }
    double getReferenceOuterRadius() const override;

    // Resolve direct children from their AxialPosition contract.  Supplying a
    // finite containerLength makes FromBottom deterministic; otherwise the
    // furthest non-bottom child defines the tail reference.
    double resolveAxialChildPositions(
        double containerLength = std::numeric_limits<double>::quiet_NaN()
    );

    static MassProperties aggregateMassProperties(const Component& root);
    static AeroProperties aggregateAeroProperties(const Component& root, const FlightCondition& fc);
    static DragBreakdown aggregateDragBreakdown(const Component& root, const FlightCondition& fc);
    static double aggregateAxialLength(const Component& root);
    DragBreakdown totalDragBreakdown(const FlightCondition& fc) const;

protected:
    MassProperties rawMassProperties() const override;
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;
    DragBreakdown dragBreakdown(const FlightCondition& fc) const override;

private:
    static void collectMass(
        const Component& component,
        const Eigen::Vector3d& origin,
        const Eigen::Matrix3d& rotation,
        MassProperties& total
    );

    static void shiftInertiaToPoint(
        const MassProperties& source,
        const Eigen::Vector3d& sourceCg,
        const Eigen::Vector3d& targetCg,
        Eigen::Matrix3d& inertia
    );

    static void collectAero(
        const Component& component,
        double zOffset,
        const FlightCondition& fc,
        AeroProperties& total,
        double& cnaCpMoment
    );

    static void collectDrag(
        const Component& component,
        const FlightCondition& fc,
        DragBreakdown& total
    );

    static void collectAxialLength(const Component& component, double zOffset, double& maxLength);
};
