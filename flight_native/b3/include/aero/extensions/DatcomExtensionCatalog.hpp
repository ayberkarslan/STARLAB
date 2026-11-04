#pragma once

#include "aero/extensions/AeroExtensionPipeline.hpp"

namespace RoketSiM::AeroExtensions {

// These entries are intentionally catalogue-only until a separately reviewed
// port supplies and validates an implementation.  Their switches already have
// stable IDs, but their production default and availability are both false.
AeroExtensionPipeline makeDatcomExtensionPipeline();
AeroExtensionSwitches defaultDatcomExtensionSwitches();
AeroExtensionConfiguration defaultDatcomExtensionConfiguration();

} // namespace RoketSiM::AeroExtensions
