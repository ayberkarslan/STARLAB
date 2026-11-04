#pragma once

#include "datcom_equivalent/body/HybridProgramData.hpp"

namespace datcom_equivalent::body_fin::program {

extern const std::vector<std::pair<std::uint32_t, std::uint32_t>> kFunctions;
extern const std::vector<std::pair<std::uint32_t, std::uint32_t>> kInitialDwords;
extern const std::vector<std::pair<std::uint32_t, std::uint32_t>> kTableDwords;
extern const std::vector<body::hybrid_program::Operation> kOperations;
extern const std::vector<std::pair<std::uint32_t, std::uint32_t>> kThunks;
extern const std::vector<std::pair<std::uint32_t, body::hybrid_program::MathIntrinsic>> kMathIntrinsics;

}  // namespace datcom_equivalent::body_fin::program
