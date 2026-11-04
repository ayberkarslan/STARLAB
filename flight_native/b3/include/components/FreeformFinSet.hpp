#pragma once

#include <string>
#include <vector>

#include "components/FinSet.hpp"

// OpenRocket freeform fin polygon.  Coordinates are (axial z, radial span)
// in the component-local plane and must describe one fin.
class FreeformFinSet : public FinSet {
public:
    FreeformFinSet(std::string name, int nFins, double thickness,
                   const Malzeme& material,
                   std::vector<Eigen::Vector2d> planform,
                   std::string crossSection = "square",
                   double cantAngle = 0.0);

    ComponentTrait traits() const noexcept override {
        return ComponentTrait::ExternalComponent
            | ComponentTrait::LeafComponent
            | ComponentTrait::FreeformFinSet;
    }

    std::vector<Eigen::Vector2d> planformPoints() const override;
    DragBreakdown dragBreakdown(const FlightCondition& fc) const override;

protected:
    AeroProperties rawAeroProperties(const FlightCondition& fc) const override;

private:
    std::vector<Eigen::Vector2d> planform_;
    double cantAngle_ = 0.0;
};
