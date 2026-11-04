#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "core/Rocket.hpp"

Rocket::Rocket(std::string name)
    : name_(std::move(name))
{
}

const std::string& Rocket::name() const noexcept {
    return name_;
}

ComponentTrait Rocket::traits() const noexcept {
    return ComponentTrait::Rocket;
}

bool Rocket::isCompatible(const Component& child) const noexcept {
    return hasTrait(child.traits(), ComponentTrait::AxialStage);
}

void Rocket::addStage(const std::shared_ptr<Stage>& stage, double zOffset) {
    if (!stage) {
        throw std::invalid_argument("Stage must not be null.");
    }
    if (!isCompatible(*stage)) {
        throw IncompatibleComponentError(name(), stage->name());
    }
    stage->localTransform.position = Eigen::Vector3d(0.0, 0.0, zOffset);
    stages_.push_back(stage);
}

const std::vector<std::shared_ptr<Stage>>& Rocket::stages() const noexcept {
    return stages_;
}

MassProperties Rocket::massProperties() const {
    MassProperties total;

    for (const auto& stage : stages_) {
        if (!stage) {
            continue;
        }

        const MassProperties stageMass = stage->massProperties();
        const Eigen::Matrix3d stageRotation = stage->localTransform.rotation;
        const Eigen::Vector3d stageCg =
            stage->localTransform.position + stageRotation * stageMass.cgLocal;
        const Eigen::Matrix3d stageInertia =
            stageRotation * stageMass.inertiaLocal * stageRotation.transpose();

        addMassAt(total, stageMass, stageCg, stageInertia);
    }

    return total;
}

void Rocket::addMassAt(
    MassProperties& total,
    const MassProperties& part,
    const Eigen::Vector3d& partCg,
    const Eigen::Matrix3d& partInertia
) {
    if (part.mass <= 0.0) {
        return;
    }

    const double newMass = total.mass + part.mass;
    const Eigen::Vector3d oldCg = total.cgLocal;
    const Eigen::Vector3d newCg = (total.mass * total.cgLocal + part.mass * partCg) / newMass;

    Eigen::Matrix3d newInertia = Eigen::Matrix3d::Zero();
    if (total.mass > 0.0) {
        const Eigen::Vector3d dOld = oldCg - newCg;
        newInertia += total.inertiaLocal
            + total.mass * (dOld.squaredNorm() * Eigen::Matrix3d::Identity() - dOld * dOld.transpose());
    }

    const Eigen::Vector3d dPart = partCg - newCg;
    newInertia += partInertia
        + part.mass * (dPart.squaredNorm() * Eigen::Matrix3d::Identity() - dPart * dPart.transpose());

    total.mass = newMass;
    total.cgLocal = newCg;
    total.inertiaLocal = newInertia;
}

AeroProperties Rocket::aeroProperties(const FlightCondition& fc) const {
    AeroProperties total;
    double cnaCpMoment = 0.0;

    for (const auto& stage : stages_) {
        if (!stage) {
            continue;
        }

        const AeroProperties stageAero = stage->aeroProperties(fc);
        const double stageOffset = stage->localTransform.position.z();
        total.cna += stageAero.cna;
        total.cd += stageAero.cd;
        total.cl += stageAero.cl;
        total.cn += stageAero.cn;
        total.cm += stageAero.cm;
        total.croll += stageAero.croll;
        total.crollDamp += stageAero.crollDamp;
        total.crollForce += stageAero.crollForce;
        cnaCpMoment += stageAero.cna * (stageOffset + stageAero.cp);
    }

    if (std::abs(total.cna) > 1e-12) {
        total.cp = cnaCpMoment / total.cna;
    }

    return total;
}

DragBreakdown Rocket::totalDragBreakdown(const FlightCondition& fc) const {
    DragBreakdown total;
    for (const auto& stage : stages_) {
        if (stage) {
            total += stage->totalDragBreakdown(fc);
        }
    }
    return total;
}

double Rocket::axialLength() const {
    double maxLength = 0.0;

    for (const auto& stage : stages_) {
        if (!stage) {
            continue;
        }
        maxLength = std::max(maxLength, stage->localTransform.position.z() + stage->axialLength());
    }

    return maxLength;
}
