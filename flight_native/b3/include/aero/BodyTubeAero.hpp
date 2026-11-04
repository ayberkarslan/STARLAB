#pragma once

#include "aero/AeroProperties.hpp"

class BodyTubeAero {
public:
    BodyTubeAero(double length, double outerRadius, double xForward);

    double localReferenceDiameter() const noexcept;
    double xcp() const noexcept;
    double cna(double machNumber = 0.0) const noexcept;
    AeroProperties properties() const noexcept;

    void printInfo() const;

private:
    double length_ = 0.0;
    double outerRadius_ = 0.0;
    double xForward_ = 0.0;
};
