#pragma once

#include <string>

#include "aero/AeroProperties.hpp"

class NoseConeAero {
public:
    NoseConeAero(double length, double baseRadius, const std::string& shape, double xForward);

    double localReferenceDiameter() const noexcept;
    double xcp() const noexcept;
    double cna(double machNumber = 0.0) const noexcept;
    AeroProperties properties() const noexcept;

    void printInfo() const;

private:
    double length_ = 0.0;
    double baseRadius_ = 0.0;
    std::string shape_ = "conical";
    double xForward_ = 0.0;
};
