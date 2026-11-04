#pragma once

#include <memory>
#include <string>
#include <vector>

#include "aero/AeroProperties.hpp"
#include "aero/DragBreakdown.hpp"
#include "aero/FlightCondition.hpp"
#include "components/Stage.hpp"
#include "mass/MassProperties.hpp"

class Rocket {
public:
    explicit Rocket(std::string name);

    const std::string& name() const noexcept;
    ComponentTrait traits() const noexcept;
    bool isCompatible(const Component& child) const noexcept;
    void addStage(const std::shared_ptr<Stage>& stage, double zOffset = 0.0);

    const std::vector<std::shared_ptr<Stage>>& stages() const noexcept;
    MassProperties massProperties() const;
    AeroProperties aeroProperties(const FlightCondition& fc) const;
    DragBreakdown totalDragBreakdown(const FlightCondition& fc) const;
    double axialLength() const;

private:
    std::string name_;
    std::vector<std::shared_ptr<Stage>> stages_;

    static void addMassAt(
        MassProperties& total,
        const MassProperties& part,
        const Eigen::Vector3d& partCg,
        const Eigen::Matrix3d& partInertia
    );
};
