#pragma once
#include <vector>
#include "datcom_integration/MappedGeometry.hpp"
namespace RoketSiM::DatcomIntegration { struct ClassifiedRequest { MappedRequest mapped; std::vector<CapabilityId> requestedCapabilities,observedCapabilities; std::vector<Diagnostic> diagnostics; }; ClassifiedRequest classifyRequest(MappedRequest mapped); }
