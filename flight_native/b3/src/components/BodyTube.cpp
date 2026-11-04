#include <cmath>
#include <iostream>

#include "components/BodyTube.hpp"
#include "aero/BodyTubeAero.hpp"
#include "aero/DragModels.hpp"

BodyTube::BodyTube(std::string name, double outerDiameter, double thickness, double length, const Malzeme& material)
    : outerDiameter_(outerDiameter),
      thickness_(thickness),
      length_(length),
      material_(material)
{
    this->name_ = std::move(name);
    if (outerDiameter_ <= 0.0) {
        throw std::invalid_argument("Outer diameter must be positive.");
    }
    if (length_ <= 0.0) {
        throw std::invalid_argument("Length must be positive.");
    }
    if (thickness_ < 0.0) {
        throw std::invalid_argument("Wall thickness must be non-negative.");
    }
    if (thickness_ >= outerRadius()) {
        throw std::invalid_argument("Wall thickness must be smaller than outer radius.");
    }
}

double BodyTube::outerDiameter() const noexcept {
    return outerDiameter_;
}

double BodyTube::outerRadius() const noexcept {
    return outerDiameter_ * 0.5;
}

double BodyTube::innerRadius() const noexcept {
    return std::max(0.0, outerRadius() - thickness_);
}

double BodyTube::wallThickness() const noexcept {
    return thickness_;
}

double BodyTube::length() const noexcept {
    return length_;
}

double BodyTube::axialLength() const {
    return length_;
}

double BodyTube::getReferenceOuterRadius() const {
    return outerRadius();
}

double BodyTube::volume() const {
    const double ro = outerRadius();
    const double ri = innerRadius();
    const double pi = std::acos(-1.0);
    return pi * (ro * ro - ri * ri) * length_;
}

MassProperties BodyTube::rawMassProperties() const {
    MassProperties props;
    props.mass = material_.alYogunluk() * volume();
    props.cgLocal = Eigen::Vector3d(0.0, 0.0, length_ / 2.0);
    const double ro = outerRadius();
    const double ri = innerRadius();
    const double Izz = 0.5 * props.mass * (ro * ro + ri * ri);
    const double Ixx = (1.0 / 12.0) * props.mass * (3.0 * (ro * ro + ri * ri) + length_ * length_);
    props.inertiaLocal(0, 0) = Ixx;
    props.inertiaLocal(1, 1) = Ixx;
    props.inertiaLocal(2, 2) = Izz;
    return props;
}

AeroProperties BodyTube::rawAeroProperties(const FlightCondition& fc) const {
    BodyTubeAero aero(length_, outerRadius(), 0.0);
    return aero.properties();
}

DragBreakdown BodyTube::dragBreakdown(const FlightCondition& fc) const {
    DragBreakdown drag;
    const double sref = DragModels::safeReferenceArea(fc);
    if (sref <= 0.0) {
        return drag;
    }

    const double wettedArea = 2.0 * std::acos(-1.0) * outerRadius() * length_;
    const double re = DragModels::reynoldsForLength(fc, length_);
    drag.friction = 1.36 * DragModels::skinFrictionCf(fc.mach, re) * wettedArea / sref;
    drag.base = DragModels::toGlobalCd(fc.motorOn ? 0.132 : 0.18, std::acos(-1.0) * outerRadius() * outerRadius(), sref);
    return drag;
}

void BodyTube::printInfo() const {
    std::cout << "=== BodyTube Info ===\n";
    std::cout << "Name: " << name() << "\n";
    std::cout << "Outer radius: " << outerRadius() << " m\n";
    std::cout << "Inner radius: " << innerRadius() << " m\n";
    std::cout << "Volume: " << volume() << " m^3\n";
    const MassProperties props = massProperties();
    std::cout << "Mass: " << props.mass << " kg\n";
    std::cout << "Ixx = Iyy = " << props.inertiaLocal(0, 0) << " kg·m^2\n";
    std::cout << "Izz = " << props.inertiaLocal(2, 2) << " kg·m^2\n";
    std::cout << "CG local = (" << props.cgLocal.x() << ", " << props.cgLocal.y() << ", " << props.cgLocal.z() << ") m\n";
}
