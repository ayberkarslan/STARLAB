#pragma once

struct AeroProperties {
    double cna = 0.0;
    double cp = 0.0;
    double cd = 0.0;
    double cl = 0.0;
    double cn = 0.0;          // Normal-force coefficient
    double cm = 0.0;          // Pitch-moment coefficient
    double croll = 0.0;       // Net roll-moment coefficient
    double crollDamp = 0.0;   // Roll damping contribution
    double crollForce = 0.0;  // Fin-cant forcing contribution
};
