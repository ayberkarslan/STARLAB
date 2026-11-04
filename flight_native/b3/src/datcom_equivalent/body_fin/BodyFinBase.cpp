#include "datcom_equivalent/body_fin/BodyFinBase.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "datcom_equivalent/body/HybridPressureMachine.hpp"
#include "datcom_equivalent/body/BodyGeometrySupport.hpp"
#include "datcom_equivalent/body_fin/BodyFinProgramData.hpp"
#include "datcom_equivalent/common/Float32.hpp"
#include "datcom_equivalent/common/Interpolation.hpp"
#include "datcom_equivalent/fin/FinStaticTables.hpp"

namespace datcom_equivalent::body_fin {
namespace {
using common::f32;
using common::f32Bits;

constexpr double kUnitM = .304878;
constexpr double kRad = 57.295780181884766;

struct Tensor {
    const std::vector<double>& a;
    const std::vector<double>& b;
    const std::vector<double>& c;
    const std::vector<double>& values;
};

double tensor(const Tensor& table, double q1, double q2, double q3) {
    std::size_t cursor = 0;
    std::vector<double> first;
    for (double ignoredA : table.a) {
        (void)ignoredA;
        std::vector<double> second;
        for (double ignoredB : table.b) {
            (void)ignoredB;
            std::vector<double> row(table.values.begin() + static_cast<long long>(cursor),
                                    table.values.begin() + static_cast<long long>(cursor + table.c.size()));
            cursor += table.c.size();
            second.push_back(common::linearInterpolateGeneral(table.c, row, q3));
        }
        first.push_back(common::linearInterpolateGeneral(table.b, second, q2));
    }
    return common::linearInterpolateGeneral(table.a, first, q1);
}

double linearCpFraction(double mach, double aspectRatio, double leadingSweepDeg, double taper) {
    using namespace fin::tables;
    const double root = [&] { const double value = std::sqrt(std::abs(mach * mach - 1.)); return value == 0. ? 1.e-30 : value; }();
    const double tangent = std::tan(leadingSweepDeg / kRad);
    const bool positive = leadingSweepDeg >= 0.;
    const bool subsonic = positive ? mach <= 1. : mach < 1.;
    const double ratio = positive ? tangent / root : std::abs(tangent / root);
    const double second = positive ? tangent * aspectRatio : std::abs(tangent * aspectRatio);
    const bool ratioLeOne = ratio <= 1.;
    const double query = ratioLeOne ? ratio : 1. / ratio;
    const Tensor* selected = nullptr;
    if (positive && subsonic && ratioLeOne) selected = new Tensor{kFinMomentCpPositiveSweepSubsonicRatioLe1Axis1,kFinMomentCpPositiveSweepSubsonicRatioLe1Axis2,kFinMomentCpPositiveSweepSubsonicRatioLe1Axis3,kFinMomentCpPositiveSweepSubsonicRatioLe1Values};
    else if (positive && subsonic) selected = new Tensor{kFinMomentCpPositiveSweepSubsonicRatioGt1Axis1,kFinMomentCpPositiveSweepSubsonicRatioGt1Axis2,kFinMomentCpPositiveSweepSubsonicRatioGt1Axis3,kFinMomentCpPositiveSweepSubsonicRatioGt1Values};
    else if (positive && ratioLeOne) selected = new Tensor{kFinMomentCpPositiveSweepSupersonicRatioLe1Axis1,kFinMomentCpPositiveSweepSupersonicRatioLe1Axis2,kFinMomentCpPositiveSweepSupersonicRatioLe1Axis3,kFinMomentCpPositiveSweepSupersonicRatioLe1Values};
    else if (positive) selected = new Tensor{kFinMomentCpPositiveSweepSupersonicRatioGt1Axis1,kFinMomentCpPositiveSweepSupersonicRatioGt1Axis2,kFinMomentCpPositiveSweepSupersonicRatioGt1Axis3,kFinMomentCpPositiveSweepSupersonicRatioGt1Values};
    else if (subsonic && ratioLeOne) selected = new Tensor{kFinMomentCpNegativeSweepSubsonicRatioLe1Axis1,kFinMomentCpNegativeSweepSubsonicRatioLe1Axis2,kFinMomentCpNegativeSweepSubsonicRatioLe1Axis3,kFinMomentCpNegativeSweepSubsonicRatioLe1Values};
    else if (subsonic) selected = new Tensor{kFinMomentCpNegativeSweepSubsonicRatioGt1Axis1,kFinMomentCpNegativeSweepSubsonicRatioGt1Axis2,kFinMomentCpNegativeSweepSubsonicRatioGt1Axis3,kFinMomentCpNegativeSweepSubsonicRatioGt1Values};
    else if (ratioLeOne) selected = new Tensor{kFinMomentCpNegativeSweepSupersonicRatioLe1Axis1,kFinMomentCpNegativeSweepSupersonicRatioLe1Axis2,kFinMomentCpNegativeSweepSupersonicRatioLe1Axis3,kFinMomentCpNegativeSweepSupersonicRatioLe1Values};
    else selected = new Tensor{kFinMomentCpNegativeSweepSupersonicRatioGt1Axis1,kFinMomentCpNegativeSweepSupersonicRatioGt1Axis2,kFinMomentCpNegativeSweepSupersonicRatioGt1Axis3,kFinMomentCpNegativeSweepSupersonicRatioGt1Values};
    const double result = tensor(*selected, taper, f32(second), f32(query));
    delete selected;
    return result;
}

bool coordinatorExternalCall(body::HybridPressureMachine& machine, std::uint32_t target) {
    if (target == 0x00401C5Du) {
        // The authoritative coordinator models this kernel call as an
        // external owner: its parent x87 state survives unchanged while only
        // the two documented REAL*4 result slots return.  Run the embedded
        // kernel in an independent machine image to preserve that boundary.
        constexpr std::uint32_t kKernelBase = 0x00620000u;
        std::unordered_map<std::uint32_t, std::uint32_t> image;
        const std::vector<double> inputs = {
            machine.indirectStackF32(0), machine.indirectStackF32(1),
            machine.indirectStackF32(2), machine.indirectStackF32(3),
            machine.indirectStackF32(4), 0., 0.,
        };
        std::vector<std::uint32_t> pointers;
        for (std::size_t i=0;i<inputs.size();++i) { const auto address=kKernelBase+static_cast<std::uint32_t>(i*4); image[address]=f32Bits(inputs[i]); pointers.push_back(address); }
        const body::HybridPressureMachine::OperationLedger kernelLedger{
            program::kFunctions, program::kInitialDwords, program::kTableDwords,
            program::kOperations, program::kThunks, program::kMathIntrinsics,
            nullptr,
        };
        body::HybridPressureMachine kernel(image, &kernelLedger);
        const auto result=kernel.run(0x00422C58u,pointers);
        machine.writeIndirectStackF32(5,result.f32(pointers[5]));
        machine.writeIndirectStackF32(6,result.f32(pointers[6]));
        return true;
    }
    if (target != 0x00401177u && target != 0x00401690u) return false;
    // The recovered helper receives pointers to mach, aspect ratio, leading
    // sweep, taper, then its REAL*4 output slot in this exact stack order.
    const double mach=machine.indirectStackF32(0), aspect=machine.indirectStackF32(1), sweep=machine.indirectStackF32(2), taper=machine.indirectStackF32(3);
    const double value=linearCpFraction(mach,aspect,sweep,taper);
    machine.writeIndirectStackF32(4, value);
    return true;
}

}  // namespace

BodyFinBaseResult evaluateBodyFinBaseInterference(
    const BodyGeometry& body, const FinGeometry& fin, const FlightCondition& flight,
    const ReferenceCondition& reference, double planformSlopePerRadian) {
    if (fin.sectionFamily != "HEX" || fin.sspanM[1] <= fin.sspanM[0] ||
        fin.chordM[0] <= 0. || fin.chordM[1] < 0. || reference.lengthM == 0.) {
        throw std::invalid_argument("unsupported BODY-FIN base-interference request");
    }
    const double unit = f32(kUnitM);
    const double xr = f32(fin.xleM[0]), xt = f32(fin.xleM[1]);
    const double sr = f32(fin.sspanM[0]), st = f32(fin.sspanM[1]);
    const double crm = f32(fin.chordM[0]), ctm = f32(fin.chordM[1]), spanM = f32(st - sr);
    const double radiusM = f32(body::bodyRadiusAtStation(body, f32(xr + f32(crm / 2.))));
    const double radius = f32(radiusM / unit), span = f32(spanM / unit), rootChord = f32(crm / unit), tipChord = f32(ctm / unit);
    const double leadingSweep = f32(std::atan(f32(f32(xt - xr) / spanM)) * kRad);
    const double trailingSweep = f32(std::atan(f32(f32(f32(xt + ctm) - f32(xr + crm)) / spanM)) * kRad);
    const double taper = f32(tipChord / rootChord), aspect = f32(f32(4. * span) / f32(rootChord + tipChord));
    const double aft = body.noseLengthM + body.centerLengthM;
    const double afterbody = f32(f32(f32(aft) - f32(xr + crm)) / unit);

    constexpr std::uint32_t kBase = 0x00610000u;
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    const std::vector<double> values = {
        f32(flight.mach), radius, span, leadingSweep, trailingSweep, rootChord,
        taper, aspect, f32(planformSlopePerRadian), afterbody,
        0., 0., 0., 0., 0., 0.,
    };
    std::vector<std::uint32_t> pointers;
    pointers.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        const std::uint32_t address = kBase + static_cast<std::uint32_t>(i * 4);
        memory[address] = f32Bits(values[i]);
        pointers.push_back(address);
    }
    const body::HybridPressureMachine::OperationLedger ledger{
        program::kFunctions, program::kInitialDwords, program::kTableDwords,
        program::kOperations, program::kThunks, program::kMathIntrinsics,
        coordinatorExternalCall,
    };
    body::HybridPressureMachine machine(memory, &ledger);
    const auto image = machine.run(0x00421db8u, pointers);
    const auto at = [&](std::size_t index) { return image.f32(pointers.at(index)); };
    const double xleInternal = f32(xr * 3.28), xcgInternal = f32(f32(reference.xcgM) * 3.28), lrefInternal = f32(f32(reference.lengthM) * 3.28);
    BodyFinBaseResult result;
    result.kwBase = at(10); result.kb = at(11); result.kkw = at(12); result.kkb = at(13);
    result.rawXcpBw = at(14); result.rawXcpWb = at(15);
    result.xcpBw = f32((result.rawXcpBw * rootChord + xleInternal - xcgInternal) / lrefInternal);
    result.xcpWb = f32((result.rawXcpWb * rootChord + xleInternal - xcgInternal) / lrefInternal);
    result.eta = image.f32(0x005D43E8u);
    result.localBodyRadiusM = f32(radius * f32(kUnitM));
    result.kernelFamily = "RECOVERED_SUB_00422C58";
    return result;
}

}  // namespace datcom_equivalent::body_fin
