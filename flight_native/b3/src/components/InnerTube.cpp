#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "components/InnerTube.hpp"

InnerTube::InnerTube(
    std::string name,
    double innerDiameter,
    double outerDiameter,
    double length,
    const Malzeme& material
)
    : innerDiameter_(innerDiameter),
      outerDiameter_(outerDiameter),
      length_(length),
      material_(material)
{
    this->name_ = std::move(name);

    if (innerDiameter_ <= 0.0) {
        throw std::invalid_argument("Inner diameter must be positive.");
    }
    if (outerDiameter_ <= 0.0) {
        throw std::invalid_argument("Outer diameter must be positive.");
    }
    if (innerDiameter_ >= outerDiameter_) {
        throw std::invalid_argument("Inner diameter must be smaller than outer diameter.");
    }
    if (length_ <= 0.0) {
        throw std::invalid_argument("Length must be positive.");
    }
}

double InnerTube::innerDiameter() const noexcept {
    return innerDiameter_;
}

double InnerTube::outerDiameter() const noexcept {
    return outerDiameter_;
}

double InnerTube::innerRadius() const noexcept {
    return innerDiameter_ * 0.5;
}

double InnerTube::outerRadius() const noexcept {
    return outerDiameter_ * 0.5;
}

double InnerTube::length() const noexcept {
    return length_;
}

double InnerTube::wallThickness() const noexcept {
    return outerRadius() - innerRadius();
}

double InnerTube::axialLength() const {
    return length_;
}

double InnerTube::getReferenceOuterRadius() const {
    return outerRadius();
}

double InnerTube::volume() const {
    const double pi = std::acos(-1.0);
    const double ro = outerRadius();
    const double ri = innerRadius();
    return pi * (ro * ro - ri * ri) * length_;
}

double InnerTube::density() const {
    return material_.alYogunluk();
}

bool InnerTube::canFitCylinder(double radius) const noexcept {
    return radius > 0.0 && radius <= innerRadius();
}

void InnerTube::addMountedComponent(std::unique_ptr<Component> child, double zOffset) {
    if (!child) {
        throw std::invalid_argument("Mounted component must not be null.");
    }
    if (zOffset < 0.0 || zOffset > length_) {
        throw std::invalid_argument("Mounted component z offset must be inside the inner tube length.");
    }
    if (zOffset + child->axialLength() > length_) {
        throw std::invalid_argument("Mounted component extends beyond the inner tube length.");
    }
    const double childRadius = child->getReferenceOuterRadius();
    if (childRadius > 0.0 && !canFitCylinder(childRadius)) {
        throw std::invalid_argument("Mounted component radius does not fit inside the inner tube.");
    }

    child->localTransform.position = Eigen::Vector3d(0.0, 0.0, zOffset);
    addChild(std::move(child));
}

MassProperties InnerTube::rawMassProperties() const {
    MassProperties props;
    props.mass = density() * volume();
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

void InnerTube::printInfo() const {
    std::cout << "=== InnerTube Info ===\n";
    std::cout << "Name: " << name() << "\n";
    std::cout << "Inner radius: " << innerRadius() << " m\n";
    std::cout << "Outer radius: " << outerRadius() << " m\n";
    std::cout << "Wall thickness: " << wallThickness() << " m\n";
    std::cout << "Length: " << length_ << " m\n";
    std::cout << "Volume: " << volume() << " m^3\n";
    std::cout << "Density: " << density() << " kg/m^3\n";
    const MassProperties props = massProperties();
    std::cout << "Mass: " << props.mass << " kg\n";
    std::cout << "Ixx = Iyy = " << props.inertiaLocal(0, 0) << " kg*m^2\n";
    std::cout << "Izz = " << props.inertiaLocal(2, 2) << " kg*m^2\n";
    std::cout << "CG local = (" << props.cgLocal.x() << ", " << props.cgLocal.y() << ", " << props.cgLocal.z() << ") m\n";
    std::cout << "Mounted children: " << children().size() << "\n";
}
