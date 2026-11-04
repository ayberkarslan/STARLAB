#include <iostream>

#include "aero/BodyTubeAero.hpp"

BodyTubeAero::BodyTubeAero(double length, double outerRadius, double xForward)
    : length_(length),
      outerRadius_(outerRadius),
      xForward_(xForward)
{
}

double BodyTubeAero::localReferenceDiameter() const noexcept {
    return 2.0 * outerRadius_;
}

double BodyTubeAero::xcp() const noexcept {
    return xForward_ + (length_ / 2.0);
}

double BodyTubeAero::cna(double /*machNumber*/) const noexcept {
    return 0.0;
}

AeroProperties BodyTubeAero::properties() const noexcept {
    AeroProperties props;
    props.cna = cna();
    props.cp = xcp();
    props.cd = 0.0;
    return props;
}

void BodyTubeAero::printInfo() const {
    std::cout << "=== Body Tube Aero Info ===\n";
    std::cout << "Xf (start)   = " << xForward_ << " m\n";
    std::cout << "Length       = " << length_ << " m\n";
    std::cout << "Outer radius = " << outerRadius_ << " m\n";
    std::cout << "Global CP    = " << xcp() << " m\n";
    std::cout << "C_Na         = " << cna() << " / rad\n";
}
