#pragma once
#include <cmath>
#include <stdexcept>
#include "datcom_equivalent/Types.hpp"
namespace RoketSiM::DatcomIntegration {
// Adapter boundary only. Numerical consumers already expect chord fractions.
inline void setSharpSymmetricDiamondSection(datcom_equivalent::FinGeometry& fin, double thicknessM) {
    if (!std::isfinite(thicknessM) || thicknessM <= 0.0)
        throw std::invalid_argument("HEX requires positive finite physical thickness");
    for (double chord : fin.chordM)
        if (!std::isfinite(chord) || chord <= 0.0)
            throw std::invalid_argument("HEX requires positive finite station chords");
    fin.sectionFamily = "HEX";
    for (int i=0; i<2; ++i) {
        fin.zupperOverChord[i] = thicknessM / (2.0 * fin.chordM[i]);
        fin.zlowerOverChord[i] = fin.zupperOverChord[i];
        fin.lmaxu[i] = 0.5;
        fin.lmaxl[i] = 0.5;
        fin.lflatu[i] = 0.0;
        fin.lflatl[i] = 0.0;
    }
    fin.lerM = {std::nullopt, std::nullopt};
}
}
