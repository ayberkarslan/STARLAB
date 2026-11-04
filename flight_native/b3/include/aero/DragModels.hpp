#pragma once

#include "aero/FlightCondition.hpp"

namespace DragModels {

double referenceAreaFromDiameter(double diameter);
double safeReferenceArea(const FlightCondition& fc);
double reynoldsForLength(const FlightCondition& fc, double length);
double skinFrictionCf(double mach, double reynolds);
double toGlobalCd(double cdLocal, double localArea, double referenceArea);
double waveDrag(double mach, double strength = 1.0);

} // namespace DragModels
