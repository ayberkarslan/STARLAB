#pragma once
#include "datcom_equivalent/body/HybridProgramData.hpp"
namespace datcom_equivalent::fin::section_program {
extern const std::vector<std::pair<std::uint32_t,std::uint32_t>> kFunctions, kInitialDwords, kTableDwords, kThunks;
extern const std::vector<body::hybrid_program::Operation> kOperations;
extern const std::vector<std::pair<std::uint32_t,body::hybrid_program::MathIntrinsic>> kMathIntrinsics;
}
