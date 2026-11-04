#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

#include "components/NoseCone.hpp"
#include "aero/DragModels.hpp"
#include "aero/NoseConeAero.hpp"

static const double PI = std::acos(-1.0);

NoseCone::NoseCone(
    std::string name,
    double baseDiameter,
    double length,
    const Malzeme& material,
    double thickness,
    std::string shape,
    double shoulderLength,
    double shoulderOuterDiameter,
    double shoulderThickness
)
    : baseDiameter_(baseDiameter),
      length_(length),
      thickness_(thickness),
      shape_(std::move(shape)),
      shoulderLength_(shoulderLength),
      shoulderOuterDiameter_(shoulderOuterDiameter),
      shoulderThickness_(shoulderThickness),
      material_(material)
{
    this->name_ = std::move(name);
    std::transform(shape_.begin(), shape_.end(), shape_.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (baseDiameter_ <= 0.0) {
        throw std::invalid_argument("Base diameter must be positive.");
    }
    if (length_ <= 0.0) {
        throw std::invalid_argument("Length must be positive.");
    }
    if (thickness_ < 0.0) {
        throw std::invalid_argument("Wall thickness must be non-negative.");
    }
    if (thickness_ >= baseRadius()) {
        throw std::invalid_argument("Wall thickness must be smaller than base radius.");
    }
    if (shoulderLength_ < 0.0) {
        throw std::invalid_argument("Shoulder length must be non-negative.");
    }
    if (shoulderThickness_ < 0.0) {
        throw std::invalid_argument("Shoulder thickness must be non-negative.");
    }
    if (shoulderOuterDiameter_ > 0.0 && shoulderThickness_ >= shoulderOuterRadius()) {
        throw std::invalid_argument("Shoulder thickness must be smaller than shoulder outer radius.");
    }
}

double NoseCone::baseDiameter() const noexcept {
    return baseDiameter_;
}

double NoseCone::baseRadius() const noexcept {
    return baseDiameter_ * 0.5;
}

double NoseCone::length() const noexcept {
    return length_;
}

double NoseCone::axialLength() const {
    return length_;
}

double NoseCone::getReferenceOuterRadius() const {
    return baseRadius();
}

double NoseCone::thickness() const noexcept {
    return thickness_;
}

std::string NoseCone::shape() const noexcept {
    return shape_;
}

double NoseCone::shoulderLength() const noexcept {
    return shoulderLength_;
}

double NoseCone::shoulderOuterDiameter() const noexcept {
    return shoulderOuterDiameter_;
}

double NoseCone::shoulderOuterRadius() const noexcept {
    return shoulderOuterDiameter_ * 0.5;
}

double NoseCone::shoulderThickness() const noexcept {
    return shoulderThickness_;
}

double NoseCone::shoulderInnerRadius() const {
    return std::max(0.0, shoulderOuterRadius() - shoulderThickness_);
}

double NoseCone::radiusAt(double z) const {
    if (z <= 0.0) {
        return 0.0;
    }
    if (z >= length_) {
        return baseRadius();
    }

    const double x = z / length_;
    const double R = baseRadius();

    if (shape_ == "ogive") {
        const double L = length_;
        const double rho = (R * R + L * L) / (2.0 * R);
        return std::sqrt(std::max(0.0, rho * rho - (L - z) * (L - z))) + R - rho;
    }

    if (shape_ == "parabolic") {
        // Python's ParabolicNoseCone with K = 0.5 default
        const double K = 0.5;
        const double numerator = 2.0 * x - K * (x * x);
        const double denominator = 2.0 - K;
        return R * (numerator / denominator);
    }

    if (shape_ == "haack" || shape_ == "von-karman" || shape_ == "ld-haack") {
        // Python's HaackNoseCone with C = 0.0 default (Von Karman)
        const double C = 0.0;
        const double theta = std::acos(1.0 - 2.0 * x);
        double inside = theta - 0.5 * std::sin(2.0 * theta) + C * std::pow(std::sin(theta), 3);
        return (R / std::sqrt(PI)) * std::sqrt(std::max(0.0, inside));
    }

    // default conical
    return R * x;
}

double NoseCone::innerRadiusAt(double z) const {
    return std::max(0.0, radiusAt(z) - thickness_);
}

double NoseCone::trapezoidIntegrate(const std::function<double(double)>& f, int points) const {
    if (points <= 0) {
        return 0.0;
    }

    const double a = 0.0;
    const double b = length_;
    const double h = (b - a) / points;
    double sum = 0.5 * (f(a) + f(b));

    for (int i = 1; i < points; ++i) {
        const double x = a + i * h;
        sum += f(x);
    }

    return sum * h;
}

double NoseCone::volume() const {
    const double noseVolume = trapezoidIntegrate([this](double z) {
        const double rOut = radiusAt(z);
        const double rIn = innerRadiusAt(z);
        return PI * (rOut * rOut - rIn * rIn);
    });

    if (shoulderLength_ <= 0.0 || shoulderOuterRadius() <= 0.0) {
        return noseVolume;
    }

    const double shoulderVolume = PI * (shoulderOuterRadius() * shoulderOuterRadius() - shoulderInnerRadius() * shoulderInnerRadius()) * shoulderLength_;
    return noseVolume + shoulderVolume;
}

double NoseCone::noseCgZ() const {
    const double noseVolume = trapezoidIntegrate([this](double z) {
        const double rOut = radiusAt(z);
        const double rIn = innerRadiusAt(z);
        return PI * (rOut * rOut - rIn * rIn);
    });

    if (noseVolume <= 0.0) {
        return 0.0;
    }

    const double numerator = trapezoidIntegrate([this](double z) {
        const double rOut = radiusAt(z);
        const double rIn = innerRadiusAt(z);
        return z * PI * (rOut * rOut - rIn * rIn);
    });

    return numerator / noseVolume;
}

double NoseCone::noseIzzCg() const {
    return trapezoidIntegrate([this](double z) {
        const double rOut = radiusAt(z);
        const double rIn = innerRadiusAt(z);
        const double dm = material_.alYogunluk() * PI * (rOut * rOut - rIn * rIn);
        return 0.5 * (rOut * rOut + rIn * rIn) * dm;
    });
}

double NoseCone::noseIxxCg() const {
    const double zCg = noseCgZ();
    return trapezoidIntegrate([this, zCg](double z) {
        const double rOut = radiusAt(z);
        const double rIn = innerRadiusAt(z);
        const double dm = material_.alYogunluk() * PI * (rOut * rOut - rIn * rIn);
        return (0.25 * (rOut * rOut + rIn * rIn) + (z - zCg) * (z - zCg)) * dm;
    });
}

double NoseCone::shoulderCgZ() const {
    return length_ + shoulderLength_ * 0.5;
}

double NoseCone::shoulderMass() const {
    return material_.alYogunluk() * (PI * (shoulderOuterRadius() * shoulderOuterRadius() - shoulderInnerRadius() * shoulderInnerRadius()) * shoulderLength_);
}

double NoseCone::shoulderIzzCg() const {
    const double m = shoulderMass();
    const double ro = shoulderOuterRadius();
    const double ri = shoulderInnerRadius();
    return 0.5 * m * (ro * ro + ri * ri);
}

double NoseCone::shoulderIxxCg() const {
    const double m = shoulderMass();
    const double ro = shoulderOuterRadius();
    const double ri = shoulderInnerRadius();
    const double L = shoulderLength_;
    return (1.0 / 12.0) * m * (3.0 * (ro * ro + ri * ri) + L * L);
}

MassProperties NoseCone::rawMassProperties() const {
    MassProperties props;

    const double noseVolume = trapezoidIntegrate([this](double z) {
        const double rOut = radiusAt(z);
        const double rIn = innerRadiusAt(z);
        return PI * (rOut * rOut - rIn * rIn);
    });
    const double noseMass = material_.alYogunluk() * noseVolume;
    const double shoulderMass = (shoulderLength_ > 0.0 && shoulderOuterRadius() > 0.0)
        ? this->shoulderMass()
        : 0.0;

    props.mass = noseMass + shoulderMass;

    const double noseCgz = noseCgZ();
    const double shoulderCgz = shoulderLength_ > 0.0 ? (length_ + shoulderLength_ * 0.5) : 0.0;

    if (props.mass > 0.0) {
        props.cgLocal = Eigen::Vector3d(
            0.0,
            0.0,
            (noseMass * noseCgz + shoulderMass * shoulderCgz) / props.mass
        );
    }

    const double zTotal = props.cgLocal.z();
    const double noseIxxCgVal = noseIxxCg();
    const double noseIzzCgVal = noseIzzCg();
    const double shoulderIxxCgVal = (shoulderMass > 0.0) ? shoulderIxxCg() : 0.0;
    const double shoulderIzzCgVal = (shoulderMass > 0.0) ? shoulderIzzCg() : 0.0;

    const double noseShift = noseMass * (noseCgz - zTotal) * (noseCgz - zTotal);
    const double shoulderShift = shoulderMass * (shoulderCgz - zTotal) * (shoulderCgz - zTotal);

    props.inertiaLocal(0, 0) = noseIxxCgVal + noseShift + shoulderIxxCgVal + shoulderShift;
    props.inertiaLocal(1, 1) = noseIxxCgVal + noseShift + shoulderIxxCgVal + shoulderShift;
    props.inertiaLocal(2, 2) = noseIzzCgVal + shoulderIzzCgVal;

    return props;
}

AeroProperties NoseCone::rawAeroProperties(const FlightCondition& fc) const {
    NoseConeAero aero(length_, baseRadius(), shape_, 0.0);
    return aero.properties();
}

DragBreakdown NoseCone::dragBreakdown(const FlightCondition& fc) const {
    DragBreakdown drag;

    const double sref = DragModels::safeReferenceArea(fc);
    if (sref <= 0.0) {
        return drag;
    }

    const double frontalArea = PI * baseRadius() * baseRadius();
    const double wettedArea = trapezoidIntegrate([this](double z) {
        const double dz = std::max(1e-6, length_ * 1e-5);
        const double z0 = std::max(0.0, z - dz);
        const double z1 = std::min(length_, z + dz);
        const double slope = (radiusAt(z1) - radiusAt(z0)) / std::max(1e-9, z1 - z0);
        return 2.0 * PI * radiusAt(z) * std::sqrt(1.0 + slope * slope);
    }, 500);

    const double re = DragModels::reynoldsForLength(fc, length_);
    drag.friction = 1.11 * DragModels::skinFrictionCf(fc.mach, re) * wettedArea / sref;

    const double fineness = length_ / std::max(baseDiameter(), 1e-9);
    const double cdPressureLocal = (shape_ == "conical" ? 0.040 : 0.018) / std::max(fineness, 1.0);
    drag.pressure = DragModels::toGlobalCd(cdPressureLocal, frontalArea, sref);
    drag.wave = DragModels::toGlobalCd(
        DragModels::waveDrag(fc.mach, 0.5 / std::max(fineness, 1.0)),
        frontalArea,
        sref
    );

    return drag;
}

void NoseCone::printInfo() const {
    std::cout << "=== NoseCone Info ===\n";
    std::cout << "Name: " << name() << "\n";
    std::cout << "Base radius: " << baseRadius() << " m\n";
    std::cout << "Length: " << length_ << " m\n";
    std::cout << "Thickness: " << thickness_ << " m\n";
    std::cout << "Volume: " << volume() << " m^3\n";
    const MassProperties props = massProperties();
    std::cout << "Mass: " << props.mass << " kg\n";
    std::cout << "Ixx = Iyy = " << props.inertiaLocal(0, 0) << " kg·m^2\n";
    std::cout << "Izz = " << props.inertiaLocal(2, 2) << " kg·m^2\n";
    std::cout << "CG local = (" << props.cgLocal.x() << ", " << props.cgLocal.y() << ", " << props.cgLocal.z() << ") m\n";
}

