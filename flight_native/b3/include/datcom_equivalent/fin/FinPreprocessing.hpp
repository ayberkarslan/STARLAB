#pragma once
#include <array>
#include <string>
#include "datcom_equivalent/Types.hpp"
namespace datcom_equivalent::fin {
struct FinPreprocessingState {
    double aspectRatio=0, nonlinearPlanformParameter=0, tipRootChordRatio=0;
    double leadingEdgeSweepDeg=0, trailingEdgeSweepDeg=0, halfChordSweepDeg=0, sectionAngleDeg=0;
    double sectionLiftSlopePerRad=0, fullExposedSpanM=0, panelAreaM2=0, nonlinearSectionLimit=0;
    double meanAerodynamicSectionChordM=0, effectiveLeRadiusM=0;
    std::array<double,50> profileX{}, profileHalfThickness{}, profileMeanLine{};
    std::array<double,7> sectionKernelOutputs{};
    std::string provenance;
};
FinPreprocessingState preprocessFinSectionState(const FinGeometry&, const FlightCondition&);
}
