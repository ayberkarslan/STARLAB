#pragma once

// Fail-closed validity gate for the DATCOM Equivalent public request contract.
//
// Faithful port of the oracle's validity taxonomy, read directly from:
//   third_party/datcom_equivalent_oracle/datcom_equivalent/validity.py
//     (require_single_finset_static_contract)
//   third_party/datcom_equivalent_oracle/datcom_equivalent/request.py
//     (DatcomEquivalentRequest.evaluate — beta/control/cant gate)
//
// These checks classify support; they must not be relaxed to make a
// downstream numerical port "pass" more cases. If a C++ parity failure
// occurs, fix the C++ translation — never the oracle or this gate.

#include <stdexcept>
#include <string>

#include "datcom_equivalent/Types.hpp"

namespace datcom_equivalent {

// Common base so callers can catch every validity rejection uniformly.
class ValidityError : public std::runtime_error {
public:
    explicit ValidityError(const std::string& what) : std::runtime_error(what) {}
};

// The requested BODY or FIN geometry is outside the recovered contract.
class UnsupportedGeometry : public ValidityError {
public:
    explicit UnsupportedGeometry(const std::string& what) : ValidityError(what) {}
};

// The requested static flight point is outside the frozen contract.
class UnsupportedFlightCondition : public ValidityError {
public:
    explicit UnsupportedFlightCondition(const std::string& what) : ValidityError(what) {}
};

// The requested configuration needs an unrecovered owner (or a native
// panel-count / beta / cant / control-deflection value outside the
// supported static-longitudinal envelope).
class UnsupportedConfiguration : public ValidityError {
public:
    explicit UnsupportedConfiguration(const std::string& what) : ValidityError(what) {}
};

// Reject inputs outside the advertised one-BODY/one-FINSET static
// envelope. Throws one of the three exception types above; returns
// normally (does nothing) if the request is within contract.
//
// Checks beta_deg / control_deflection_deg / fin_cant_deg first (mirrors
// DatcomEquivalentRequest.evaluate() in request.py, which gates before
// delegating to the provider), then the geometry/flight-condition gate
// (mirrors require_single_finset_static_contract in validity.py), in the
// exact source order: BODY positivity -> FIN family -> NPANEL -> boundary
// layer -> Mach range -> alpha range -> diameter range.
void validateRequest(const DatcomEquivalentRequest& request);

}  // namespace datcom_equivalent
