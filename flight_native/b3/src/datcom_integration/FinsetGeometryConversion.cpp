#include "datcom_integration/FinsetGeometryConversion.hpp"

#include <stdexcept>

#include "datcom_integration/HexSectionContract.hpp"

namespace RoketSiM::DatcomIntegration {
namespace {

std::vector<double> panelAzimuths(int count, double orientationDeg) {
    std::vector<double> values;
    values.reserve(count > 0 ? static_cast<std::size_t>(count) : 0U);
    if (count <= 0) return values;
    const double first = 180.0 / static_cast<double>(count) + orientationDeg;
    for (int index = 0; index < count; ++index)
        values.push_back(first + 360.0 * index / count);
    return values;
}

}  // namespace

std::vector<ProviderFinsetGeometry> convertMappedFinsets(const MappedRequest& mapped) {
    std::vector<ProviderFinsetGeometry> converted;
    converted.reserve(mapped.finsets.size());
    const double diameter = mapped.body.diameterM > 0.0 ? mapped.body.diameterM
        : (mapped.bodySections.empty() ? 0.0 : mapped.bodySections.front().foreDiameterM);

    for (const auto& fin : mapped.finsets) {
        ProviderFinsetGeometry item;
        item.sourceComponentId = fin.sourceComponentId;
        item.importedSourceComponentId = fin.importedSourceComponentId;
        item.axialLocationM = fin.axialLocationM;
        item.orientationDeg = fin.orientationDeg;
        item.sectionFamily = fin.sectionFamily;
        item.sectionSupport = fin.sectionSupport;
        // Both component types consume the same proof result. Freeform legacy
        // scalars are deliberately never a fallback for missing source stations.
        if (fin.exactPlanform) {
            const auto& planform = *fin.exactPlanform;
            item.geometry.sspanM = {diameter * .5, diameter * .5 + planform.spanM};
            item.geometry.chordM = {planform.rootChordM, planform.tipChordM};
            item.geometry.xleM = planform.leadingEdgeM;
        }
        item.geometry.panelAzimuthsDeg = panelAzimuths(fin.panelCount, fin.orientationDeg);
        item.geometry.npanel = fin.panelCount;

        item.hasExactProviderGeometry = fin.sectionSupport == FinSectionSupport::Exact
            && fin.exactPlanform.has_value();
        if (item.hasExactProviderGeometry) {
            // The provider consumes dimensionless half-thickness/chord
            // ordinates; apply that contract independently to every FINSET.
            try {
                setSharpSymmetricDiamondSection(item.geometry, fin.thicknessM);
            } catch (const std::invalid_argument&) {
                item.hasExactProviderGeometry = false;
            }
        }
        converted.push_back(std::move(item));
    }
    return converted;
}

}  // namespace RoketSiM::DatcomIntegration
