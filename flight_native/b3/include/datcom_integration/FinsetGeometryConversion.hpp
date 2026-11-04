#pragma once

#include <string>
#include <vector>

#include "datcom_equivalent/Types.hpp"
#include "datcom_integration/MappedGeometry.hpp"

namespace RoketSiM::DatcomIntegration {

// Integration-owned, ordered conversion boundary.  This deliberately does
// not expand the frozen one-FINSET numerical request: a later coordinator can
// consume this collection only after it defines multi-FINSET physics.
struct ProviderFinsetGeometry {
    std::string sourceComponentId;
    std::string importedSourceComponentId;
    double axialLocationM = 0.0;
    double orientationDeg = 0.0;
    FinSectionFamily sectionFamily = FinSectionFamily::Unknown;
    FinSectionSupport sectionSupport = FinSectionSupport::NotImplemented;
    bool hasExactProviderGeometry = false;
    datcom_equivalent::FinGeometry geometry;
};

// Preserves MappedRequest::finsets source order.  Unsupported sections or
// planforms retain their identity but do not claim an exact provider geometry;
// raw parameters remain in MappedRequest. Only ExactFinPlanform supplies
// provider stations; diagnostics remain owned by RequestClassifier.
std::vector<ProviderFinsetGeometry> convertMappedFinsets(const MappedRequest& mapped);

}  // namespace RoketSiM::DatcomIntegration
