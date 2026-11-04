#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>

#include "aero/NoseConeAero.hpp"

NoseConeAero::NoseConeAero(double length, double baseRadius, const std::string& shape, double xForward)
    : length_(length),
      baseRadius_(baseRadius),
      shape_(shape),
      xForward_(xForward)
{
    std::transform(shape_.begin(), shape_.end(), shape_.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
}

double NoseConeAero::localReferenceDiameter() const noexcept {
    return 2.0 * baseRadius_;
}

double NoseConeAero::xcp() const noexcept {
    double localCp = 0.5 * length_;

    if (shape_.find("conical") != std::string::npos) {
        localCp = 0.666 * length_;
    } else if (shape_.find("ogive") != std::string::npos) {
        // Slender-body/Barrowman CP: x_cp = L - V/(pi R^2).  The historical
        // 0.466*L approximation is not exact for every tangent-ogive ratio.
        if (baseRadius_ > 0.0 && length_ > 0.0) {
            const double rho = (baseRadius_ * baseRadius_ + length_ * length_)
                / (2.0 * baseRadius_);
            constexpr int slices = 2000;
            const double dz = length_ / slices;
            double volume = 0.0;
            for (int i = 0; i < slices; ++i) {
                const double z = (i + 0.5) * dz;
                const double radius = std::sqrt(std::max(
                    rho * rho - (length_ - z) * (length_ - z), 0.0))
                    + baseRadius_ - rho;
                volume += std::acos(-1.0) * radius * radius * dz;
            }
            localCp = length_ - volume /
                (std::acos(-1.0) * baseRadius_ * baseRadius_);
        }
    } else if (shape_.find("parabolic") != std::string::npos) {
        localCp = 0.500 * length_;
    } else if (shape_.find("haack") != std::string::npos) {
        localCp = 0.437 * length_;
    } else if (shape_.find("power") != std::string::npos) {
        localCp = 0.500 * length_;
    }

    return xForward_ + localCp;
}

double NoseConeAero::cna(double /*machNumber*/) const noexcept {
    return 2.0;
}

AeroProperties NoseConeAero::properties() const noexcept {
    AeroProperties props;
    props.cna = cna();
    props.cp = xcp();
    props.cd = 0.0;
    return props;
}

void NoseConeAero::printInfo() const {
    std::cout << "=== Nose Cone Aero Info ===\n";
    std::cout << "Shape        = " << shape_ << "\n";
    std::cout << "Xf (Start)   = " << xForward_ << " m\n";
    std::cout << "Length       = " << length_ << " m\n";
    std::cout << "Base Radius  = " << baseRadius_ << " m\n";
    std::cout << "Global CP    = " << xcp() << " m\n";
    std::cout << "C_Na         = " << cna() << " / rad\n";
}
