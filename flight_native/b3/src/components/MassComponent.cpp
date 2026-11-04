#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "components/MassComponent.hpp"

MassComponent::MassComponent(std::string name, double mass, double radius, double length)
    : mass_(mass),
      radius_(radius),
      length_(length)
{
    this->name_ = std::move(name);

    if (mass_ <= 0.0) {
        throw std::invalid_argument("Mass must be positive.");
    }
    if (radius_ <= 0.0) {
        throw std::invalid_argument("Radius must be positive.");
    }
    if (length_ <= 0.0) {
        throw std::invalid_argument("Length must be positive.");
    }
}

double MassComponent::mass() const noexcept {
    return mass_;
}

double MassComponent::radius() const noexcept {
    return radius_;
}

double MassComponent::length() const noexcept {
    return length_;
}

double MassComponent::volume() const {
    const double pi = std::acos(-1.0);
    return pi * radius_ * radius_ * length_;
}

double MassComponent::density() const {
    return mass_ / volume();
}

double MassComponent::axialLength() const {
    return length_;
}

double MassComponent::getReferenceOuterRadius() const {
    return radius_;
}

MassProperties MassComponent::rawMassProperties() const {
    MassProperties props;
    props.mass = mass_;
    props.cgLocal = Eigen::Vector3d(0.0, 0.0, length_ / 2.0);

    const double Izz = 0.5 * mass_ * radius_ * radius_;
    const double Ixx = (1.0 / 12.0) * mass_ * (3.0 * radius_ * radius_ + length_ * length_);

    props.inertiaLocal(0, 0) = Ixx;
    props.inertiaLocal(1, 1) = Ixx;
    props.inertiaLocal(2, 2) = Izz;
    return props;
}

void MassComponent::printInfo() const {
    std::cout << "=== MassComponent Info ===\n";
    std::cout << "Name: " << name() << "\n";
    std::cout << "Radius: " << radius_ << " m\n";
    std::cout << "Length: " << length_ << " m\n";
    std::cout << "Volume: " << volume() << " m^3\n";
    std::cout << "Density: " << density() << " kg/m^3\n";
    const MassProperties props = massProperties();
    std::cout << "Mass: " << props.mass << " kg\n";
    std::cout << "Ixx = Iyy = " << props.inertiaLocal(0, 0) << " kg*m^2\n";
    std::cout << "Izz = " << props.inertiaLocal(2, 2) << " kg*m^2\n";
    std::cout << "CG local = (" << props.cgLocal.x() << ", " << props.cgLocal.y() << ", " << props.cgLocal.z() << ") m\n";
}
