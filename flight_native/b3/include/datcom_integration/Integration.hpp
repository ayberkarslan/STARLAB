#pragma once
#include "datcom_equivalent/Types.hpp"
#include "datcom_integration/FinsetGeometryConversion.hpp"
#include "datcom_integration/RequestClassifier.hpp"
#include "nlohmann/json.hpp"
namespace RoketSiM::DatcomIntegration { datcom_equivalent::DatcomEquivalentRequest compatibilityRequest(const ClassifiedRequest& classified); nlohmann::json classify(const nlohmann::json& input); nlohmann::json execute(const nlohmann::json& input); }
