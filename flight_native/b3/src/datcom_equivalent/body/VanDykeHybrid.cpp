#include "datcom_equivalent/body/VanDykeHybrid.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <unordered_map>
#include <vector>

#include "datcom_equivalent/body/BodyNormal.hpp"  // BodyMethodNotImplemented
#include "datcom_equivalent/body/HybridPressureMachine.hpp"
#include "datcom_equivalent/common/Float32.hpp"

namespace datcom_equivalent::body {

namespace {

using datcom_equivalent::common::f32;

constexpr double kDegreesPerRadianReal4 = 57.295780181884766;
constexpr double kPiReal4 = 3.1415927410125732;  // f32(math.pi)
constexpr double kFourReal4 = 4.0;
constexpr double kTinyReal4 = 1.0000000031710769e-30;
// These mirror the oracle's own f32()-truncated envelope constants
// (hybrid.py: `MIN_SUPERSONIC_MACH_REAL4 = f32(1.2)` etc) exactly -- NOT
// the raw decimal literals. 0.05 and 0.4 are not exactly representable in
// binary32, so f32(0.4) = 0.4000000059604645 != 0.4; comparing against
// the raw literal instead of the f32-rounded value incorrectly rejects a
// diameter of exactly 0.4 m (and, for kMinSupersonicMach, incorrectly
// routes a stored-REAL4-exact Mach of 1.2 to the supersonic path instead
// of the low-Mach owner). Values below are static_cast<float> of the
// decimal literal, which is a valid constexpr conversion.
constexpr double kMinSupersonicMach = static_cast<float>(1.2);
constexpr double kMaxSupportedMach = static_cast<float>(5.0);
constexpr double kMinConeFineness = static_cast<float>(1.5);
constexpr double kMaxConeFineness = static_cast<float>(8.0);
constexpr double kMinCenterFineness = static_cast<float>(1.0);
constexpr double kMaxCenterFineness = static_cast<float>(10.0);
constexpr double kMinDiameter = static_cast<float>(0.05);
constexpr double kMaxDiameter = static_cast<float>(0.4);

std::uint32_t fbits(double value) {
    float f = static_cast<float>(f32(value));
    std::uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}

// _normalized_cone_cylinder_contour: 11 nose stations + 10 new cylinder
// stations, normalized so the cone spans [0, noseFineness] and the
// cylinder continues to [noseFineness, noseFineness + centerFineness],
// with radius normalized to a unit-diameter (0.5 max radius) body.
struct Contour {
    std::vector<double> x;
    std::vector<double> r;
};

Contour normalizedConeCylinderContour(double noseFineness, double centerFineness) {
    Contour c;
    c.x.reserve(21);
    c.r.reserve(21);
    for (int index = 0; index < 11; ++index) {
        c.x.push_back(f32(static_cast<double>(index) * noseFineness / 10.0));
        c.r.push_back(f32(static_cast<double>(index) * 0.5 / 10.0));
    }
    for (int index = 1; index < 11; ++index) {
        c.x.push_back(f32(noseFineness + static_cast<double>(index) * centerFineness / 10.0));
        c.r.push_back(f32(0.5));
    }
    return c;
}

HybridScalarResult evaluateHybridScalarUncached(double noseLength, double diameter,
                                                 double centerLength, double mach,
                                                 double reynoldsOperand, double sref, double lref,
                                                 double xcgFromNose) {
    noseLength = f32(noseLength);
    diameter = f32(diameter);
    centerLength = f32(centerLength);
    mach = f32(mach);
    const double noseFineness = f32(noseLength / diameter);
    const double centerFineness = f32(centerLength / diameter);
    const Contour contour = normalizedConeCylinderContour(noseFineness, centerFineness);
    const std::size_t stationCount = contour.x.size();  // 21

    std::vector<std::uint32_t> pointers;
    pointers.reserve(15);
    for (int index = 0; index < 15; ++index) pointers.push_back(0x71000000u + 0x400u * index);

    const std::uint32_t requestedMethod = 2;
    const std::uint32_t words[15] = {
        requestedMethod, 0, 0, fbits(noseFineness), fbits(mach),
        fbits(reynoldsOperand), 0, fbits(diameter), 0, 0,
        static_cast<std::uint32_t>(stationCount), 0, 0, 0, 0,
    };

    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    for (int i = 0; i < 15; ++i) memory[pointers[i]] = words[i];
    memory[pointers[6]] = fbits(f32(1.0) / kDegreesPerRadianReal4);
    for (std::size_t i = 0; i < stationCount; ++i) {
        memory[pointers[1] + 4u * static_cast<std::uint32_t>(i)] = fbits(contour.x[i]);
    }
    for (std::size_t i = 0; i < stationCount; ++i) {
        memory[pointers[2] + 4u * static_cast<std::uint32_t>(i)] = fbits(contour.r[i]);
    }
    const std::uint32_t tiny = fbits(kTinyReal4);
    for (std::uint32_t address = 0x0062AAA0u; address < 0x0062E660u; address += 4u) {
        memory[address] = tiny;
    }
    memory[0x0063BBB8u] = 0;
    memory[0x0063BBBCu] = fbits(noseLength);
    memory[0x0063BBCCu] = fbits(centerLength);
    memory[0x0063BBD4u] = tiny;
    memory[0x0063BBD8u] = tiny;

    HybridPressureMachine machine(memory);
    const auto result = machine.run(0x004A6520u, pointers);

    auto readRaw = [](const std::unordered_map<std::uint32_t, std::uint32_t>& mem,
                       std::uint32_t address) -> std::uint32_t {
        auto it = mem.find(address);
        return it == mem.end() ? 0u : it->second;
    };

    const std::uint32_t failure = readRaw(result.memory, pointers[14]);

    double axialWork;
    double axialAlphaWork;
    double cnSlope;
    double cmWork;
    std::string actualMethod;
    bool fallbackEntered;
    std::size_t instructionCount = result.instructionCount;

    if (failure != 0) {
        // Hybrid signaled failure: the native caller responds by invoking
        // SOSE (0x004F9E80) with the contour state prepared by the gate,
        // continuing from Hybrid's own memory image (not a fresh machine).
        std::unordered_map<std::uint32_t, std::uint32_t> fallbackMemory = result.memory;
        for (std::size_t i = 0; i < stationCount; ++i) {
            fallbackMemory[0x005FBAB8u + 4u * static_cast<std::uint32_t>(i)] = fbits(contour.x[i]);
        }
        for (std::size_t i = 0; i < stationCount; ++i) {
            fallbackMemory[0x005FB9F0u + 4u * static_cast<std::uint32_t>(i)] = fbits(contour.r[i]);
        }
        fallbackMemory[0x005FB990u] = static_cast<std::uint32_t>(stationCount);
        fallbackMemory[0x005FB9A0u] = 11u;
        fallbackMemory[0x005FB9A4u] = 10u;
        fallbackMemory[0x005FB970u] = 2u;
        fallbackMemory[0x005FB95Cu] = 0u;
        fallbackMemory[0x005FB958u] = 2u;
        fallbackMemory[0x00635D60u] = fbits(diameter);
        fallbackMemory[0x00635D64u] = fbits(sref);
        fallbackMemory[0x00635D68u] = fbits(f32(noseLength + centerLength));
        fallbackMemory[0x72000000u] = 0u;
        fallbackMemory[0x72000400u] = 0u;

        const std::vector<std::uint32_t> fallbackArguments = {
            0x005C174Cu, pointers[4], 0x005FBAB8u, 0x005FB9F0u, 0x005FB990u,
            0x005FB9A0u, 0x005FB970u, 0x005FB95Cu, 0x005FB958u, 0x72000000u,
            0x005FB934u, pointers[12], pointers[13], 0x72000400u, 0x00635D60u,
        };
        HybridPressureMachine fallbackMachine(fallbackMemory);
        const auto fallback = fallbackMachine.run(0x004F9E80u, fallbackArguments);

        // SOSE returns per-radian work values; unlike the successful
        // VANDYK return, the caller does not apply the degree-to-radian
        // factor here.
        cnSlope = fallback.f32(pointers[12]);
        cmWork = fallback.f32(pointers[13]);
        axialWork = fallback.f32(0x72000000u);
        axialAlphaWork = fallback.f32(0x72000400u);
        actualMethod = "SECOND_ORDER_SHOCK_EXPANSION";
        fallbackEntered = true;
        instructionCount += fallback.instructionCount;
    } else {
        // sub_005108A8 passes its CA-pressure work slot as argument 12 of
        // sub_004A6520, immediately before the already exposed CN/CM outputs.
        axialWork = result.f32(pointers[11]);
        axialAlphaWork = f32(0.0);
        const double rawCnWork = result.f32(pointers[12]);
        const double rawCmWork = result.f32(pointers[13]);
        cnSlope = f32(kDegreesPerRadianReal4 * rawCnWork);
        cmWork = f32(kDegreesPerRadianReal4 * rawCmWork);
        actualMethod = "VAN_DYKE_HYBRID";
        fallbackEntered = false;
    }

    const double areaScale = f32(diameter * kPiReal4 * diameter / kFourReal4 / f32(sref));
    const double pressureWaveCa0 = f32(axialWork * areaScale);
    const double pressureWaveAlphaScalar = f32(axialAlphaWork * areaScale);
    const double potentialCnScalar = f32(cnSlope * areaScale);
    const double normalizedDirectCm =
        f32(diameter * kPiReal4 * diameter * diameter / kFourReal4 * cmWork / (f32(lref) * f32(sref)));
    const double potentialCmScalar =
        f32(normalizedDirectCm + f32(xcgFromNose) * potentialCnScalar / f32(lref));

    HybridScalarResult out;
    out.pressureWaveCa0 = pressureWaveCa0;
    out.pressureWaveAlphaScalar = pressureWaveAlphaScalar;
    out.potentialCnScalar = potentialCnScalar;
    out.potentialCmScalar = potentialCmScalar;
    out.hybridSuccess = (failure == 0);
    out.failureSentinel = failure;
    out.actualMethod = actualMethod;
    out.fallbackEntered = fallbackEntered;
    out.contourStationCount = static_cast<int>(stationCount);
    out.instructionCount = instructionCount;
    return out;
}

// Mirrors hybrid.py's `@lru_cache(maxsize=256)` on `_evaluate_hybrid_cached`:
// the Hybrid/SOSE scalar state depends only on geometry+Mach+Reynolds+
// reference quantities, never on alpha (alpha folding happens entirely
// outside this function, in BodySupersonic.cpp's foldedPotential calls),
// so repeated calls at the same operating point are memoized. This is a
// pure performance optimization matching the oracle's own behavior, not a
// numerical shortcut: a cache hit returns exactly the value an uncached
// call would compute, since evaluateHybridScalarUncached is a
// deterministic pure function of its 8 arguments. Unlike the oracle's
// bounded 256-entry LRU, this cache is unbounded for this port (unbounded
// growth is not a concern at the scale this owner is exercised at); noted
// as a deliberate simplification of the *eviction policy* only, not the
// memoized value itself.
struct HybridCacheKey {
    std::uint32_t bits[8];
    bool operator==(const HybridCacheKey& other) const {
        return std::equal(std::begin(bits), std::end(bits), std::begin(other.bits));
    }
};
struct HybridCacheKeyHash {
    std::size_t operator()(const HybridCacheKey& key) const {
        std::size_t h = 0;
        for (std::uint32_t b : key.bits) h ^= std::hash<std::uint32_t>{}(b) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

HybridScalarResult evaluateHybridScalar(double noseLength, double diameter, double centerLength,
                                         double mach, double reynoldsOperand, double sref,
                                         double lref, double xcgFromNose) {
    static std::unordered_map<HybridCacheKey, HybridScalarResult, HybridCacheKeyHash> cache;
    const HybridCacheKey key{{fbits(noseLength), fbits(diameter), fbits(centerLength),
                               fbits(mach), fbits(reynoldsOperand), fbits(sref), fbits(lref),
                               fbits(xcgFromNose)}};
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    HybridScalarResult result = evaluateHybridScalarUncached(
        noseLength, diameter, centerLength, mach, reynoldsOperand, sref, lref, xcgFromNose);
    cache.emplace(key, result);
    return result;
}

}  // namespace

void validateSupportedSupersonicEnvelope(const BodyGeometry& body, const FlightCondition& flight,
                                          const ReferenceCondition& reference,
                                          std::optional<double> alphaDeg) {
    const double mach = f32(flight.mach);
    if (!(kMinSupersonicMach < mach && mach <= kMaxSupportedMach)) {
        throw BodyMethodNotImplemented(
            "translated supersonic BODY range is stored REAL4 1.2f < Mach <= 5.0");
    }
    // nose_type=="CONE"/bluntness==0/center_diameter==diameter are
    // structurally guaranteed by BodyGeometry's shape -- see BodyNormal.cpp
    // for the same reasoning applied to the M<=1.2 owner.
    const double noseLength = f32(body.noseLengthM);
    const double diameter = f32(body.diameterM);
    const double centerLength = f32(body.centerLengthM);
    if (diameter <= 0.0 || noseLength <= 0.0 || centerLength <= 0.0) {
        throw BodyMethodNotImplemented(
            "nose length, center length, and diameter must be positive");
    }
    const double noseFineness = f32(noseLength / diameter);
    const double centerFineness = f32(centerLength / diameter);
    if (!(kMinConeFineness <= noseFineness && noseFineness <= kMaxConeFineness)) {
        throw BodyMethodNotImplemented(
            "validated sharp-cone fineness range is 1.5 through 8 diameters");
    }
    if (!(kMinCenterFineness <= centerFineness && centerFineness <= kMaxCenterFineness)) {
        throw BodyMethodNotImplemented(
            "validated center-body length range is 1 through 10 diameters");
    }
    if (!(kMinDiameter <= diameter && diameter <= kMaxDiameter)) {
        throw BodyMethodNotImplemented("validated body-diameter range is 0.05 through 0.4 m");
    }
    if (alphaDeg.has_value() && std::abs(f32(*alphaDeg)) > f32(10.0)) {
        throw BodyMethodNotImplemented(
            "validated supersonic angle-of-attack range is -10 through 10 degrees");
    }
    if (f32(reference.areaM2) <= 0.0 || f32(reference.lengthM) <= 0.0) {
        throw BodyMethodNotImplemented("Sref and Lref must be positive");
    }
}

HybridScalarResult evaluateVanDykeHybridSharpCone(const BodyGeometry& body,
                                                    const FlightCondition& flight,
                                                    const ReferenceCondition& reference) {
    validateSupportedSupersonicEnvelope(body, flight, reference);  // no alpha check here
    const double diameter = f32(body.diameterM);
    const double noseOrigin = f32(body.noseOriginM);
    const double reynoldsPerM = f32(flight.reynoldsPerM);
    const double reynoldsOperand = f32((reynoldsPerM / f32(3.28)) / f32(flight.mach));
    return evaluateHybridScalar(f32(body.noseLengthM), diameter, f32(body.centerLengthM),
                                 f32(flight.mach), reynoldsOperand, f32(reference.areaM2),
                                 f32(reference.lengthM), f32(reference.xcgM - noseOrigin));
}

}  // namespace datcom_equivalent::body
