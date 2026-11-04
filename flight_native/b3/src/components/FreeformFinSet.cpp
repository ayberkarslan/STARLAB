#include "components/FreeformFinSet.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "aero/DragModels.hpp"
#include "aero/FinSetBarrowman.hpp"
#include "aero/FinPlanform.hpp"

FreeformFinSet::FreeformFinSet(std::string name, int nFins, double thickness,
                               const Malzeme& material,
                               std::vector<Eigen::Vector2d> planform,
                               std::string crossSection,
                               double cantAngle)
    : FinSet(std::move(name), nFins, thickness, material, std::move(crossSection)),
      planform_(std::move(planform)),
      cantAngle_(cantAngle) {
    if (planform_.size() < 3) {
        throw std::invalid_argument("A FreeformFinSet requires at least three planform points.");
    }
}

std::vector<Eigen::Vector2d> FreeformFinSet::planformPoints() const {
    return planform_;
}

DragBreakdown FreeformFinSet::dragBreakdown(const FlightCondition& fc) const {
    std::vector<FinPlanform::Point> points;
    points.reserve(planform_.size());
    for (const auto& point : planform_) points.push_back({point.x(), point.y()});
    const FinPlanform::Metrics metrics = FinPlanform::calculate(points, 48);
    const double referenceArea = DragModels::safeReferenceArea(fc);
    if (!metrics.valid || referenceArea <= 1.0e-14) return {};

    DragBreakdown drag;
    const double reynolds = DragModels::reynoldsForLength(fc, metrics.macLength);
    const double skinFriction = DragModels::skinFrictionCf(fc.mach, reynolds);
    drag.friction = numFins() * skinFriction
        * (1.0 + 2.0 * thickness() / metrics.macLength)
        * 2.0 * metrics.area / referenceArea;

    const std::string section = crossSection();
    if (section == "rounded" || section == "airfoil") {
        const double mach = std::max(fc.mach, 0.0);
        double coefficient;
        if (mach < 0.9) {
            coefficient = std::pow(std::max(1.0 - mach * mach, 1.0e-14), -0.417) - 1.0;
        } else if (mach < 1.0) {
            coefficient = 1.0 - 1.785 * (mach - 0.9);
        } else {
            const double mach2 = mach * mach;
            coefficient = 1.214 - 0.502 / mach2 + 0.1095 / (mach2 * mach2);
        }
        drag.pressure = numFins() * coefficient
            * metrics.leadingEdgeCosine * metrics.leadingEdgeCosine
            * metrics.span * thickness() / referenceArea;
        drag.fin = drag.pressure;
    } else {
        const DragBreakdown fallback = FinSet::dragBreakdown(fc);
        drag.pressure = fallback.pressure;
        drag.base = fallback.base;
        drag.fin = fallback.fin;
        drag.wave = fallback.wave;
    }
    return drag;
}
AeroProperties FreeformFinSet::rawAeroProperties(
    const FlightCondition& fc
) const {
    std::vector<FinPlanform::Point> points;
    points.reserve(planform_.size());
    for (const auto& point : planform_) {
        points.push_back({point.x(), point.y()});
    }
    const FinPlanform::Metrics metrics = FinPlanform::calculate(points);
    AeroProperties result;
    if (!metrics.valid) {
        return result;
    }

    const double bodyRadius = getReferenceOuterRadius();
    const double referenceDiameter = 2.0 * bodyRadius;
    const double localReferenceArea = std::acos(-1.0)
        * referenceDiameter * referenceDiameter / 4.0;
    const double referenceArea = fc.referenceArea > 1.0e-14
        ? fc.referenceArea : localReferenceArea;
    if (referenceArea <= 1.0e-14) {
        return result;
    }
    const double aspectRatio =
        2.0 * metrics.span * metrics.span / metrics.area;
    result.cna = FinSetBarrowman::aggregateCna(
        metrics.area, metrics.span, metrics.midChordCosine,
        std::max(fc.mach, 0.0), fc.alpha, referenceArea,
        numFins(), bodyRadius);
    result.cp = metrics.macLeadingEdge
        + FinSetBarrowman::cpFraction(aspectRatio, std::max(fc.mach, 0.0))
            * metrics.macLength;

    const double limitedAlpha = FinSetBarrowman::limitedAngleOfAttack(fc.alpha);
    result.cn = FinSetBarrowman::normalForceCoefficient(result.cna, fc.alpha);
    const double referenceLength = fc.referenceLength > 1.0e-14
        ? fc.referenceLength : std::max(fc.referenceDiameter, 2.0 * bodyRadius);
    if (referenceLength > 1.0e-14) {
        result.cm = result.cn * result.cp / referenceLength;
        const double oneFinCna = FinSetBarrowman::oneFinCna(
            metrics.area, metrics.span, metrics.midChordCosine,
            std::max(fc.mach, 0.0), fc.alpha, referenceArea);
        const double tau = bodyRadius > 0.0
            ? bodyRadius / (bodyRadius + metrics.span) : 0.0;
        result.crollForce = (metrics.centroidY + bodyRadius) * oneFinCna
            * (1.0 + tau) * cantAngle_ / referenceLength;
        if (limitedAlpha >= FinSetBarrowman::kStallAngle) {
            const double reduction = std::clamp(
                1.0 - (std::abs(fc.alpha) - FinSetBarrowman::kStallAngle)
                    / (FinSetBarrowman::kStallAngle / 2.0),
                0.0, 1.0);
            result.crollForce *= reduction;
        }

        const double speed = std::abs(fc.velocity);
        if (std::abs(fc.rollRate) >= 0.1 && speed > 1.0e-9
            && !metrics.stripY.empty()) {
            const double dy = metrics.span / metrics.stripY.size();
            auto dampingAtMach = [&](double mach) {
                double sum = 0.0;
                const bool stalledTip = std::abs(fc.rollRate)
                    * (bodyRadius + metrics.span) / speed
                    > 15.0 * FinSetBarrowman::kPi / 180.0;
                if (stalledTip || mach <= FinSetBarrowman::kSubsonicLimit) {
                    const double beta = std::sqrt(std::max(
                        std::abs(1.0 - mach * mach), 1.0e-12));
                    for (std::size_t i = 0; i < metrics.stripY.size(); ++i) {
                        const double distance = bodyRadius + metrics.stripY[i];
                        const double angle = stalledTip
                            ? std::min(std::abs(fc.rollRate) * distance / speed,
                                15.0 * FinSetBarrowman::kPi / 180.0)
                            : fc.rollRate * distance / speed;
                        sum += metrics.stripChord[i] * distance * angle;
                    }
                    const double sign = stalledTip
                        ? std::copysign(1.0, fc.rollRate) : 1.0;
                    return sign * sum * dy * 2.0 * FinSetBarrowman::kPi
                        / (beta * referenceArea * referenceLength);
                }
                const auto k = FinSetBarrowman::supersonicCoefficients(mach);
                for (std::size_t i = 0; i < metrics.stripY.size(); ++i) {
                    const double distance = bodyRadius + metrics.stripY[i];
                    const double angle = fc.rollRate * distance / speed;
                    sum += (k.k1 * angle + k.k2 * angle * angle
                        + k.k3 * angle * angle * angle)
                        * metrics.stripChord[i] * distance;
                }
                return sum * dy / (referenceArea * referenceLength);
            };
            const double mach = std::max(fc.mach, 0.0);
            if (mach <= FinSetBarrowman::kSubsonicLimit
                || mach >= FinSetBarrowman::kSupersonicLimit) {
                result.crollDamp = dampingAtMach(mach);
            } else {
                const double fraction = (mach - FinSetBarrowman::kSubsonicLimit)
                    / (FinSetBarrowman::kSupersonicLimit
                        - FinSetBarrowman::kSubsonicLimit);
                result.crollDamp = (1.0 - fraction)
                    * dampingAtMach(FinSetBarrowman::kSubsonicLimit - 0.01)
                    + fraction
                    * dampingAtMach(FinSetBarrowman::kSupersonicLimit + 0.01);
            }
        }
        result.croll = result.crollForce - result.crollDamp;
    }
    return result;
}
