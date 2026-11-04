#pragma once
#include <string>
namespace RoketSiM::DatcomIntegration { enum class AnalysisMode { StaticLongitudinal, StaticDerivatives, DynamicDerivatives, MagnusSpin, TrimPitch, HypersBodyOnly, BasePlume, Inlet }; struct MappedCondition { AnalysisMode analysisMode=AnalysisMode::StaticLongitudinal; double mach=0.0,alphaDeg=0.0,betaDeg=0.0,altitudeM=0.0,reynoldsPerM=0.0; std::string boundaryLayer="TURB"; double roughnessM=0.0,finCantDeg=0.0,controlDeflectionDeg=0.0; }; std::string toString(AnalysisMode value); AnalysisMode analysisModeFromString(const std::string& value); }
