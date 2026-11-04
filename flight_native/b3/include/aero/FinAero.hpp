#pragma once

#include "aero/AeroProperties.hpp"
#include "aero/FlightCondition.hpp"
#include <string>

class FinAero {
public:
    FinAero(double xForward, double sweepLength, double rootChord, double tipChord, double span, double bodyRadius, int finCount);

    double localReferenceDiameter() const noexcept;
    double xcpLocal() const noexcept;
    double xcpGlobal() const noexcept;
    double mac() const noexcept;
    double cna(double machNumber = 0.0) const noexcept;
    
    AeroProperties properties(const FlightCondition& fc) const noexcept;

private:
    double xf_ = 0.0;
    double xr_ = 0.0;
    double cr_ = 0.0;
    double ct_ = 0.0;
    double s_ = 0.0;
    double bodyRadius_ = 0.0;
    int finCount_ = 0;

    double lam_ = 0.0;
    double mac_ = 0.0;
    double x_mac_ = 0.0;
    double y_mac_ = 0.0;
    double xcp_local_ = 0.0;
    double xcp_global_ = 0.0;

    void update();
    bool isValidGeometry() const;
};
