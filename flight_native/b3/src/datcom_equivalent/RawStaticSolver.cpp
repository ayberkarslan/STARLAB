#include "datcom_equivalent/RawStaticSolver.hpp"

#include "datcom_equivalent/Validity.hpp"
#include "datcom_equivalent/body/BodyAxial.hpp"
#include "datcom_equivalent/body/BodySupersonic.hpp"
#include "datcom_equivalent/body_fin/BodyFinBase.hpp"
#include "datcom_equivalent/body_fin/BodyFinFinite.hpp"
#include "datcom_equivalent/body_fin/BodyFinSynthesis.hpp"
#include "datcom_equivalent/common/Float32.hpp"
#include "datcom_equivalent/fin/FinPreprocessing.hpp"
#include "datcom_equivalent/fin/IsolatedFinStatic.hpp"

namespace datcom_equivalent {

DatcomEquivalentResult evaluateRawStaticLongitudinal(const DatcomEquivalentRequest& request) {
    using common::f32;
    validateRequest(request);
    const auto normal=body::evaluateBodyStaticNormalMoment(request.body,request.flight,request.reference,request.alphaDeg);
    const auto axial=body::evaluateAxisymmetricBodyStaticAxial(request.body,request.flight,request.reference,request.alphaDeg);
    const auto state=fin::preprocessFinSectionState(request.fin,request.flight);
    const auto isolated=fin::evaluateIsolatedFinStatic(request.fin,state,request.flight,request.reference,request.alphaDeg);
    const auto base=body_fin::evaluateBodyFinBaseInterference(request.body,request.fin,request.flight,request.reference,isolated.cnaPanelPerRadian);
    const auto finite=body_fin::evaluateBodyFinFiniteAlpha(request.body,request.fin,request.flight,request.reference,state,base,request.alphaDeg,normal.bdwk);
    const double finsetCa=f32(isolated.ca0Panel*request.fin.npanel);
    const auto total=body_fin::synthesizeBodyFinStatic(
        {normal.cnTotal,normal.cmTotal,axial.caTotal},
        {isolated.cn,isolated.cm,finsetCa},finite.kwAlpha,base.kb,base.xcpWb,base.xcpBw,
        finite.finInBodyCn,finsetCa);
    DatcomEquivalentResult out;
    out.finalCa=total.total.ca;out.finalCn=total.total.cn;out.finalCm=total.total.cm;
    out.bodyCa=axial.caTotal;out.bodyCn=normal.cnTotal;out.bodyCm=normal.cmTotal;
    out.isolatedFinCa=finsetCa;out.isolatedFinCna=isolated.cnaPanelPerRadian;out.isolatedFinCn=isolated.cn;out.isolatedFinCm=isolated.cm;out.isolatedFinCp=isolated.cpM;
    out.kwBase=base.kwBase;out.kw=finite.kwAlpha;out.kb=base.kb;out.kkw=base.kkw;out.kkb=base.kkb;out.bdwk=normal.bdwk;
    for(const auto& panel:finite.panels){out.equivalentPanelAnglesDeg.push_back(panel.equivalentAlphaDeg);out.panelCn.push_back(panel.cnPanel);}
    out.finInBodyCn=finite.finInBodyCn;out.finInBodyCa=finsetCa;out.carryoverCn=total.bodyCarryover.cn;out.carryoverCm=total.bodyCarryover.cm;
    out.activeBodyMethod=normal.activeMethod;out.bodyFallback=normal.methodFallback;
    out.validity=request.body.noseFamily==BodyNoseFamily::TangentOgive
        ? "VALID_TANGENT_OGIVE_SINGLE_FINSET_SUBTRANS_STATIC_RAW_OWNER_COMPOSITION"
        : "VALID_SHARP_CONE_SINGLE_FINSET_STATIC_RAW_OWNER_COMPOSITION";
    out.warnings.insert(out.warnings.end(),normal.warnings.begin(),normal.warnings.end());out.warnings.insert(out.warnings.end(),axial.warnings.begin(),axial.warnings.end());
    return out;
}

}  // namespace datcom_equivalent
