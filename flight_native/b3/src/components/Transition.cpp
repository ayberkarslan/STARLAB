#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "aero/DragModels.hpp"
#include "aero/Barrowman.hpp"
#include "components/Transition.hpp"

static const double PI = std::acos(-1.0);

Transition::Transition(
    std::string name,
    double foreDiameter,
    double aftDiameter,
    double length,
    double thickness,
    const Malzeme& material
)
    : foreDiameter_(foreDiameter),
      aftDiameter_(aftDiameter),
      length_(length),
      thickness_(thickness),
      material_(material)
{
    this->name_ = std::move(name);

    if (foreDiameter_ <= 0.0) {
        throw std::invalid_argument("Fore diameter must be positive.");
    }
    if (aftDiameter_ <= 0.0) {
        throw std::invalid_argument("Aft diameter must be positive.");
    }
    if (length_ <= 0.0) {
        throw std::invalid_argument("Length must be positive.");
    }
    if (thickness_ < 0.0) {
        throw std::invalid_argument("Wall thickness must be non-negative.");
    }
    if (thickness_ >= std::min(foreRadius(), aftRadius())) {
        throw std::invalid_argument("Wall thickness must be smaller than both end radii.");
    }
}

double Transition::foreDiameter() const noexcept {
    return foreDiameter_;
}

double Transition::aftDiameter() const noexcept {
    return aftDiameter_;
}

double Transition::foreRadius() const noexcept {
    return foreDiameter_ * 0.5;
}

double Transition::aftRadius() const noexcept {
    return aftDiameter_ * 0.5;
}

double Transition::length() const noexcept {
    return length_;
}

double Transition::thickness() const noexcept {
    return thickness_;
}

double Transition::axialLength() const {
    return length_;
}

double Transition::getReferenceOuterRadius() const {
    return std::max(foreRadius(), aftRadius());
}

double Transition::innerForeRadius() const {
    return std::max(0.0, foreRadius() - thickness_);
}

double Transition::innerAftRadius() const {
    return std::max(0.0, aftRadius() - thickness_);
}

double Transition::outerRadiusAt(double z) const {
    return foreRadius() + (aftRadius() - foreRadius()) * (z / length_);
}

double Transition::innerRadiusAt(double z) const {
    return innerForeRadius() + (innerAftRadius() - innerForeRadius()) * (z / length_);
}

double Transition::solidFrustumVolume(double r1, double r2) const {
    return (PI * length_ / 3.0) * (r1 * r1 + r1 * r2 + r2 * r2);
}

double Transition::solidFrustumCgZ(double r1, double r2) const {
    const double denom = r1 * r1 + r1 * r2 + r2 * r2;
    if (denom <= 0.0) {
        return 0.0;
    }
    return length_ * (r1 * r1 + 2.0 * r1 * r2 + 3.0 * r2 * r2) / (4.0 * denom);
}

double Transition::volume() const {
    const double outerVolume = solidFrustumVolume(foreRadius(), aftRadius());
    const double innerVolume = solidFrustumVolume(innerForeRadius(), innerAftRadius());
    return outerVolume - innerVolume;
}

double Transition::cgZ() const {
    const double outerVolume = solidFrustumVolume(foreRadius(), aftRadius());
    const double innerVolume = solidFrustumVolume(innerForeRadius(), innerAftRadius());
    const double totalVolume = outerVolume - innerVolume;

    if (totalVolume <= 0.0) {
        return 0.0;
    }

    const double outerCg = solidFrustumCgZ(foreRadius(), aftRadius());
    const double innerCg = solidFrustumCgZ(innerForeRadius(), innerAftRadius());
    return (outerVolume * outerCg - innerVolume * innerCg) / totalVolume;
}

double Transition::cpZ() const {
    return Barrowman::transitionCpLocal(
        foreRadius(), aftRadius(), length_);
}

double Transition::cna(double referenceRadius) const {
    return Barrowman::transitionCna(
        foreRadius(), aftRadius(), referenceRadius);
}

double Transition::referenceRadiusFrom(const FlightCondition& fc) const {
    if (fc.referenceDiameter > 0.0) {
        return fc.referenceDiameter * 0.5;
    }
    if (fc.referenceArea > 0.0) {
        return std::sqrt(fc.referenceArea / PI);
    }
    if (parent) {
        const double parentRadius = parent->getReferenceOuterRadius();
        if (parentRadius > 0.0) {
            return parentRadius;
        }
    }
    return getReferenceOuterRadius();
}

double Transition::integrate(const std::function<double(double)>& f, int slices) const {
    if (slices <= 0) {
        return 0.0;
    }

    const double dz = length_ / slices;
    double sum = 0.0;

    for (int i = 0; i < slices; ++i) {
        const double z = (i + 0.5) * dz;
        sum += f(z) * dz;
    }

    return sum;
}

MassProperties Transition::rawMassProperties() const {
    MassProperties props;
    props.mass = material_.alYogunluk() * volume();
    props.cgLocal = Eigen::Vector3d(0.0, 0.0, cgZ());

    const double zcg = props.cgLocal.z();
    props.inertiaLocal(2, 2) = integrate([this](double z) {
        const double ro = outerRadiusAt(z);
        const double ri = innerRadiusAt(z);
        return 0.5 * material_.alYogunluk() * PI * (std::pow(ro, 4) - std::pow(ri, 4));
    });

    const double Ixx = integrate([this, zcg](double z) {
        const double ro = outerRadiusAt(z);
        const double ri = innerRadiusAt(z);
        const double dVdz = PI * (ro * ro - ri * ri);
        const double dmdz = material_.alYogunluk() * dVdz;
        return dmdz * (0.25 * (ro * ro + ri * ri) + (z - zcg) * (z - zcg));
    });

    props.inertiaLocal(0, 0) = Ixx;
    props.inertiaLocal(1, 1) = Ixx;
    return props;
}

AeroProperties Transition::rawAeroProperties(const FlightCondition& fc) const {
    AeroProperties props;
    props.cp = cpZ();
    props.cna = cna(referenceRadiusFrom(fc));
    props.cd = 0.0;
    return props;
}

DragBreakdown Transition::dragBreakdown(const FlightCondition& fc) const {
    DragBreakdown drag;
    const double sref = DragModels::safeReferenceArea(fc);
    if (sref <= 0.0) {
        return drag;
    }
    const double wettedArea = integrate([this](double z) {
        const double drdz = (aftRadius() - foreRadius()) / length_;
        return 2.0 * PI * outerRadiusAt(z) * std::sqrt(1.0 + drdz * drdz);
    }, 500);
    const double re = DragModels::reynoldsForLength(fc, length_);
    drag.friction = DragModels::skinFrictionCf(fc.mach, re) * wettedArea / sref;

    const double foreArea = PI * foreRadius() * foreRadius();
    const double aftArea = PI * aftRadius() * aftRadius();
    const double maxArea = std::max(foreArea, aftArea);
    const double areaDelta = std::abs(aftArea - foreArea);
    drag.pressure = DragModels::toGlobalCd(0.04 * areaDelta / std::max(maxArea, 1e-9), maxArea, sref);

    if (aftArea < foreArea) {
        drag.base = DragModels::toGlobalCd(fc.motorOn ? 0.05 : 0.12, foreArea - aftArea, sref);
    }
    drag.wave = DragModels::toGlobalCd(DragModels::waveDrag(fc.mach, 0.25), maxArea, sref);
    return drag;
}

void Transition::printInfo() const {
    std::cout << "=== Transition Info ===\n";
    std::cout << "Name: " << name() << "\n";
    std::cout << "Fore radius: " << foreRadius() << " m\n";
    std::cout << "Aft radius: " << aftRadius() << " m\n";
    std::cout << "Length: " << length_ << " m\n";
    std::cout << "Thickness: " << thickness_ << " m\n";
    std::cout << "Volume: " << volume() << " m^3\n";
    const MassProperties props = massProperties();
    std::cout << "Mass: " << props.mass << " kg\n";
    std::cout << "Ixx = Iyy = " << props.inertiaLocal(0, 0) << " kg*m^2\n";
    std::cout << "Izz = " << props.inertiaLocal(2, 2) << " kg*m^2\n";
    std::cout << "CG local = (" << props.cgLocal.x() << ", " << props.cgLocal.y() << ", " << props.cgLocal.z() << ") m\n";
}
