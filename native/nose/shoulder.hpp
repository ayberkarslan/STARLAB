#pragma once
#include "shell.hpp"
namespace starlab_nose {
// RoketSiM analyzeComponent(Nose Cone), axial shoulder and closing cap.
inline void addShoulder(Properties& result,double density,double shoulderLength,double shoulderOuter,double shoulderThickness,bool capped) {
 const double shoulderInner=shoulderOuter-shoulderThickness;
        auto addAxialRing = [&](double ringOuter, double ringInner,
                                 double ringLength, double ringCg) {
            if (ringLength <= 0.0 || ringOuter <= 0.0) return;
            const double ringVolume = kPi * (ringOuter * ringOuter
                - ringInner * ringInner) * ringLength;
            const double ringMass = density * ringVolume;
            if (ringMass <= 0.0) return;
            const double oldMass = result.mass;
            const double oldCg = result.localCg;
            const double combinedMass = oldMass + ringMass;
            const double combinedCg = (oldMass * oldCg + ringMass * ringCg)
                / combinedMass;
            result.iAxial += 0.5 * ringMass * (ringOuter * ringOuter + ringInner * ringInner);
            result.iTransverse += ringMass * (
                3.0 * (ringOuter * ringOuter + ringInner * ringInner)
                + ringLength * ringLength) / 12.0;
            result.iTransverse += oldMass * (oldCg - combinedCg) * (oldCg - combinedCg)
                + ringMass * (ringCg - combinedCg) * (ringCg - combinedCg);
            result.volume += ringVolume;
            result.mass = combinedMass;
            result.localCg = combinedCg;
        };
        addAxialRing(shoulderOuter, shoulderInner, shoulderLength,
            result.length + shoulderLength / 2.0);
        if (capped) {
            addAxialRing(shoulderInner, 0.0, shoulderThickness,
                result.length + shoulderLength - shoulderThickness / 2.0);
        }
}
}
