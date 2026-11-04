#include <algorithm>
#include <cmath>
#include <utility>

#include "components/Stage.hpp"

Stage::Stage(std::string name) {
    this->name_ = std::move(name);
}

double Stage::axialLength() const {
    return aggregateAxialLength(*this);
}

double Stage::getReferenceOuterRadius() const {
    double radius = 0.0;
    for (const auto& child : children()) {
        if (child) {
            radius = std::max(radius, child->getReferenceOuterRadius());
        }
    }
    return radius;
}

MassProperties Stage::rawMassProperties() const {
    return aggregateMassProperties(*this);
}

AeroProperties Stage::rawAeroProperties(const FlightCondition& fc) const {
    return aggregateAeroProperties(*this, fc);
}

DragBreakdown Stage::dragBreakdown(const FlightCondition& fc) const {
    return aggregateDragBreakdown(*this, fc);
}

DragBreakdown Stage::totalDragBreakdown(const FlightCondition& fc) const {
    return aggregateDragBreakdown(*this, fc);
}

MassProperties Stage::aggregateMassProperties(const Component& root) {
    MassProperties total;
    collectMass(root, Eigen::Vector3d::Zero(), Eigen::Matrix3d::Identity(), total);
    return total;
}

void Stage::collectMass(
    const Component& component,
    const Eigen::Vector3d& origin,
    const Eigen::Matrix3d& rotation,
    MassProperties& total
) {
    for (const auto& child : component.children()) {
        if (!child) {
            continue;
        }

        const Eigen::Matrix3d childRotation = rotation * child->localTransform.rotation;
        const Eigen::Vector3d childOrigin = origin + rotation * child->localTransform.position;
        const MassProperties childMass = child->massProperties();
        const Eigen::Vector3d childCg = childOrigin + childRotation * childMass.cgLocal;
        const double newMass = total.mass + childMass.mass;

        if (childMass.mass > 0.0 && newMass > 0.0) {
            const Eigen::Vector3d oldCg = total.cgLocal;
            const Eigen::Vector3d newCg =
                (total.mass * total.cgLocal + childMass.mass * childCg) / newMass;

            Eigen::Matrix3d shifted = Eigen::Matrix3d::Zero();
            if (total.mass > 0.0) {
                shiftInertiaToPoint(total, oldCg, newCg, shifted);
            }

            MassProperties childInRoot;
            childInRoot.mass = childMass.mass;
            childInRoot.cgLocal = childCg;
            childInRoot.inertiaLocal = childRotation * childMass.inertiaLocal * childRotation.transpose();
            shiftInertiaToPoint(childInRoot, childCg, newCg, shifted);

            total.mass = newMass;
            total.cgLocal = newCg;
            total.inertiaLocal = shifted;
        }

        collectMass(*child, childOrigin, childRotation, total);
    }
}

void Stage::shiftInertiaToPoint(
    const MassProperties& source,
    const Eigen::Vector3d& sourceCg,
    const Eigen::Vector3d& targetCg,
    Eigen::Matrix3d& inertia
) {
    const Eigen::Vector3d d = sourceCg - targetCg;
    inertia += source.inertiaLocal
        + source.mass * (d.squaredNorm() * Eigen::Matrix3d::Identity() - d * d.transpose());
}

AeroProperties Stage::aggregateAeroProperties(const Component& root, const FlightCondition& fc) {
    AeroProperties total;
    double cnaCpMoment = 0.0;
    collectAero(root, 0.0, fc, total, cnaCpMoment);

    if (std::abs(total.cna) > 1e-12) {
        total.cp = cnaCpMoment / total.cna;
    }

    return total;
}

void Stage::collectAero(
    const Component& component,
    double zOffset,
    const FlightCondition& fc,
    AeroProperties& total,
    double& cnaCpMoment
) {
    for (const auto& child : component.children()) {
        if (!child) {
            continue;
        }

        const double childOffset = zOffset + child->localTransform.position.z();
        const AeroProperties childAero = child->aeroProperties(fc);
        total.cna += childAero.cna;
        total.cd += childAero.cd;
        total.cl += childAero.cl;
        total.cn += childAero.cn;
        total.cm += childAero.cm;
        total.croll += childAero.croll;
        total.crollDamp += childAero.crollDamp;
        total.crollForce += childAero.crollForce;
        cnaCpMoment += childAero.cna * (childOffset + childAero.cp);

        collectAero(*child, childOffset, fc, total, cnaCpMoment);
    }
}

DragBreakdown Stage::aggregateDragBreakdown(const Component& root, const FlightCondition& fc) {
    DragBreakdown total;
    collectDrag(root, fc, total);
    return total;
}

void Stage::collectDrag(
    const Component& component,
    const FlightCondition& fc,
    DragBreakdown& total
) {
    for (const auto& child : component.children()) {
        if (!child) {
            continue;
        }
        total += child->dragBreakdown(fc);
        collectDrag(*child, fc, total);
    }
}

double Stage::resolveAxialChildPositions(double containerLength) {
    double sequentialTail = 0.0;
    double furthestKnownTail = 0.0;

    for (const auto& child : children()) {
        if (!child || child->axialPosition().mode == AxialPositionMode::FromBottom) {
            continue;
        }

        const AxialPosition placement = child->axialPosition();
        double position = placement.offset;
        if (placement.mode == AxialPositionMode::After) {
            position = sequentialTail + placement.offset;
        }
        child->localTransform.position.z() = position;
        sequentialTail = std::max(sequentialTail, position + child->axialLength());
        furthestKnownTail = std::max(furthestKnownTail, position + child->axialLength());
    }

    const double tailReference = std::isfinite(containerLength)
        ? std::max(0.0, containerLength)
        : furthestKnownTail;
    double furthestTail = furthestKnownTail;
    for (const auto& child : children()) {
        if (!child || child->axialPosition().mode != AxialPositionMode::FromBottom) {
            continue;
        }
        const AxialPosition placement = child->axialPosition();
        const double position = tailReference - child->axialLength() - placement.offset;
        child->localTransform.position.z() = position;
        furthestTail = std::max(furthestTail, position + child->axialLength());
    }
    return furthestTail;
}

double Stage::aggregateAxialLength(const Component& root) {
    double maxLength = 0.0;
    collectAxialLength(root, 0.0, maxLength);
    return maxLength;
}

void Stage::collectAxialLength(const Component& component, double zOffset, double& maxLength) {
    for (const auto& child : component.children()) {
        if (!child) {
            continue;
        }

        const double childOffset = zOffset + child->localTransform.position.z();
        maxLength = std::max(maxLength, childOffset + child->axialLength());
        collectAxialLength(*child, childOffset, maxLength);
    }
}
