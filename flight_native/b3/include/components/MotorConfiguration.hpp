#pragma once

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "components/MotorCluster.hpp"

// Flight-specific motor selection owned by a MotorMount.  The mass-changing
// MotorComponent remains the physical child in the component tree.
struct MotorConfiguration {
    std::string designation;
    double ignitionTimeS = 0.0;
    bool enabled = true;

    // Catalog and flight metadata adapted from the richer Sevda/OpenRocket
    // model without importing its unfinished component hierarchy.
    std::string manufacturer;
    std::vector<double> standardDelaysS;
    double ejectionDelayS = 0.0;
    std::string caseInfo;
    std::string propellantInfo;
    std::string dataSource;
    bool sparky = false;
    bool available = true;
    MotorClusterConfiguration cluster;

    void validate() const {
        if (designation.empty()) {
            throw std::invalid_argument("Motor configuration needs a designation.");
        }
        if (!std::isfinite(ignitionTimeS) || ignitionTimeS < 0.0) {
            throw std::invalid_argument("Motor ignition time must be non-negative.");
        }
        if (!std::isfinite(ejectionDelayS) || ejectionDelayS < 0.0) {
            throw std::invalid_argument("Motor ejection delay must be non-negative.");
        }
        for (const double delay : standardDelaysS) {
            if (!std::isfinite(delay) || delay < 0.0) {
                throw std::invalid_argument("Standard motor delays must be non-negative.");
            }
        }
        cluster.validate();
    }
};