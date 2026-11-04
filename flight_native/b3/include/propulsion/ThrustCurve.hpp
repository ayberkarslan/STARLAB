#pragma once

#include <string>
#include <vector>

struct EngMotorInfo {
    std::string designation;
    double diameterMm = 0.0;
    double lengthMm = 0.0;
    std::string delays;
    std::vector<double> standardDelaysS;
    double propellantMass = 0.0;
    double totalMass = 0.0;
    std::string manufacturer;
    std::string caseInfo;
    std::string propellantInfo;
    std::string dataSource;
    bool sparky = false;
    bool available = true;
};

class ThrustCurve {
public:
    std::vector<double> time;
    std::vector<double> thrust;
    EngMotorInfo motorInfo;

    static ThrustCurve loadEngFile(const std::string& path);

    double getThrust(double t) const;
    double impulseUntil(double t) const;
    double impulseBetween(double t0, double t1) const;
    double burnStartTime() const;
    double burnEndTime() const;
    double totalImpulse() const;
    void precompute() const;

private:
    void ensurePrecomputed() const;
    void validate() const;

    mutable std::vector<double> segmentSlope_;
    mutable std::vector<double> cumulativeImpulse_;
};