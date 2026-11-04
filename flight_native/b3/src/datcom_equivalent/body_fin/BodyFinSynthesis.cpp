#include "datcom_equivalent/body_fin/BodyFinSynthesis.hpp"

#include <stdexcept>

#include "datcom_equivalent/common/Float32.hpp"

namespace datcom_equivalent::body_fin {

BodyFinSynthesisResult synthesizeBodyFinStatic(
    const BodyFinCoefficientState& bodyAlone,
    const BodyFinCoefficientState& isolatedFin,
    double kw, double kb, double xcpWb, double xcpBw, double finInBodyCn,
    double finInBodyCa, double finInBodyDirectCm, double carryoverDirectCm,
    double carryoverCa) {
    using common::f32;
    if(kw==0.)throw std::invalid_argument("KW is zero; native KB/KW carryover quotient is undefined");
    BodyFinSynthesisResult out;
    out.finInBody.cn=f32(finInBodyCn);
    out.finInBody.cm=f32(-out.finInBody.cn*f32(xcpWb)+f32(finInBodyDirectCm));
    out.finInBody.ca=f32(finInBodyCa);
    out.bodyCarryover.cn=f32(out.finInBody.cn*f32(kb)/f32(kw));
    out.bodyCarryover.cm=f32(-out.bodyCarryover.cn*f32(xcpBw)+f32(carryoverDirectCm));
    out.bodyCarryover.ca=f32(carryoverCa);
    out.total.cn=f32(f32(bodyAlone.cn)+out.finInBody.cn+out.bodyCarryover.cn);
    out.total.cm=f32(f32(bodyAlone.cm)+out.finInBody.cm+out.bodyCarryover.cm);
    out.total.ca=f32(f32(bodyAlone.ca)+out.finInBody.ca+out.bodyCarryover.ca);
    out.interactionCn=f32(out.finInBody.cn+out.bodyCarryover.cn-f32(isolatedFin.cn));
    out.interactionCm=f32(out.finInBody.cm+out.bodyCarryover.cm-f32(isolatedFin.cm));
    if(out.total.cn!=0.)out.effectiveCp=-out.total.cm/out.total.cn;
    return out;
}

}  // namespace datcom_equivalent::body_fin
