#include <cmath>
#include <iostream>

#include "aero/FinAero.hpp"

FinAero::FinAero(double xForward, double sweepLength, double rootChord, double tipChord, double span, double bodyRadius, int finCount)
    : xf_(xForward), xr_(sweepLength), cr_(rootChord), ct_(tipChord), s_(span), bodyRadius_(bodyRadius), finCount_(finCount)
{
    update();
}

double FinAero::localReferenceDiameter() const noexcept {
    return 2.0 * bodyRadius_;
}

bool FinAero::isValidGeometry() const {
    if (xf_ < 0.0) return false;
    if (xr_ < 0.0) return false;
    if (cr_ <= 0.0) return false;
    if (ct_ < 0.0) return false;
    if (s_ <= 0.0) return false;
    if ((cr_ + ct_) <= 0.0) return false;
    return true;
}

void FinAero::update() {
    if (!isValidGeometry()) {
        lam_ = 0.0;
        mac_ = 0.0;
        x_mac_ = 0.0;
        y_mac_ = 0.0;
        xcp_local_ = 0.0;
        xcp_global_ = xf_;
        return;
    }

    lam_ = ct_ / cr_;
    mac_ = (2.0 / 3.0) * cr_ * ((1.0 + lam_ + lam_ * lam_) / (1.0 + lam_));
    y_mac_ = (s_ / 3.0) * ((1.0 + 2.0 * lam_) / (1.0 + lam_));
    x_mac_ = (xr_ / 3.0) * ((1.0 + 2.0 * lam_) / (1.0 + lam_));
    
    xcp_local_ = x_mac_ + 0.25 * mac_;
    xcp_global_ = xf_ + xcp_local_;
}

double FinAero::xcpLocal() const noexcept {
    return xcp_local_;
}

double FinAero::xcpGlobal() const noexcept {
    return xcp_global_;
}

double FinAero::mac() const noexcept {
    return mac_;
}

double FinAero::cna(double machNumber) const noexcept {
    if (!isValidGeometry() || finCount_ < 3) {
        return 0.0;
    }

    double beta = 1.0;
    if (machNumber >= 0.0 && machNumber < 1.0) {
        beta = std::sqrt(1.0 - machNumber * machNumber);
        // Avoid division by zero very close to Mach 1
        if (beta < 0.01) beta = 0.01;
    }

    double mid_chord_root = xr_ + (ct_ / 2.0);
    double mid_chord_tip = cr_ / 2.0;
    double Lf = std::sqrt(s_ * s_ + (mid_chord_root - mid_chord_tip) * (mid_chord_root - mid_chord_tip));

    double ref_diameter = localReferenceDiameter();
    if (ref_diameter <= 0.0) return 0.0;

    double term1 = 4.0 * finCount_ * (s_ / ref_diameter) * (s_ / ref_diameter);
    double term2 = 1.0 + std::sqrt(1.0 + ((2.0 * Lf) / (cr_ + ct_)) * ((2.0 * Lf) / (cr_ + ct_)));
    
    double cna_isolated = term1 / term2;
    double k_fb = 1.0 + (bodyRadius_ / (s_ + bodyRadius_));

    return (cna_isolated * k_fb) / beta;
}

AeroProperties FinAero::properties(const FlightCondition& fc) const noexcept {
    AeroProperties props;
    props.cp = xcpGlobal();
    props.cna = cna(fc.mach);
    // Base drag estimation or isolated drag can be added later
    props.cd = 0.0; 
    return props;
}
