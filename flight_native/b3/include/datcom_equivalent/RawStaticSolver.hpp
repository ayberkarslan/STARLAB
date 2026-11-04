#pragma once

#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent {

// Complete raw one-BODY/one-FINSET static-longitudinal orchestrator.  This
// composes native owners only; callers supply geometry and flight state, not
// captured coefficients or interference factors.
DatcomEquivalentResult evaluateRawStaticLongitudinal(const DatcomEquivalentRequest&);

}  // namespace datcom_equivalent
