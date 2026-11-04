#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <cctype>

#include "components/FinSet.hpp"
#include "aero/DragModels.hpp"
#include "aero/FinAero.hpp"

// FinSet Implementation

FinSet::FinSet(std::string name, int n_fins, double thickness, const Malzeme& material, std::string cross_section)
    : n_fins_(n_fins), thickness_(thickness), material_(material), cross_section_(std::move(cross_section))
{
    this->name_ = std::move(name);
    if (n_fins_ <= 0) throw std::invalid_argument("Number of fins must be positive.");
    if (thickness_ <= 0.0) throw std::invalid_argument("Thickness must be positive.");
    
    std::transform(cross_section_.begin(), cross_section_.end(), cross_section_.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
}

double FinSet::kSection() const {
    if (cross_section_ == "square") return 1.0;
    if (cross_section_ == "rounded") return 0.85;
    if (cross_section_ == "airfoil") return 0.65;
    if (cross_section_ == "diamond") return 0.5;
    if (cross_section_ == "biconvex") return 0.6;
    return 1.0;
}

double FinSet::surfaceDensity() const {
    return material_.alYogunluk() * thickness_ * kSection();
}

double FinSet::getReferenceOuterRadius() const {
    if (parent) {
        return parent->getReferenceOuterRadius();
    }
    return 0.0;
}

std::vector<Eigen::Vector2d> FinSet::closedPoints() const {
    auto pts = planformPoints();
    if (pts.size() < 3) {
        throw std::runtime_error("A fin polygon needs at least 3 points.");
    }
    if (!pts.front().isApprox(pts.back())) {
        pts.push_back(pts.front());
    }
    return pts;
}

double FinSet::polygonSignedArea() const {
    auto pts = closedPoints();
    double area = 0.0;
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        double z1 = pts[i].x(); // z
        double r1 = pts[i].y(); // r
        double z2 = pts[i+1].x();
        double r2 = pts[i+1].y();
        area += z1 * r2 - z2 * r1;
    }
    return 0.5 * area;
}

double FinSet::planformArea() const {
    return std::abs(polygonSignedArea());
}

Eigen::Vector2d FinSet::centroid() const {
    auto pts = closedPoints();
    double cz = 0.0;
    double cr = 0.0;
    double area = 0.0;
    
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        double z1 = pts[i].x();
        double r1 = pts[i].y();
        double z2 = pts[i+1].x();
        double r2 = pts[i+1].y();
        
        double cross = z1 * r2 - z2 * r1;
        area += cross;
        cz += (z1 + z2) * cross;
        cr += (r1 + r2) * cross;
    }
    
    area *= 0.5;
    if (std::abs(area) < 1e-12) {
        return Eigen::Vector2d(0.0, 0.0);
    }
    
    cz /= (6.0 * area);
    cr /= (6.0 * area);
    return Eigen::Vector2d(cz, cr);
}

std::tuple<double, double, double> FinSet::areaSecondMomentsAboutOrigin() const {
    auto pts = closedPoints();
    double Izz_area = 0.0;
    double Irr_area = 0.0;
    double Izr_area = 0.0;
    
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        double z1 = pts[i].x();
        double r1 = pts[i].y();
        double z2 = pts[i+1].x();
        double r2 = pts[i+1].y();
        
        double cross = z1 * r2 - z2 * r1;
        
        Izz_area += (r1 * r1 + r1 * r2 + r2 * r2) * cross / 12.0;
        Irr_area += (z1 * z1 + z1 * z2 + z2 * z2) * cross / 12.0;
        Izr_area += (z1 * r2 + 2.0 * z1 * r1 + 2.0 * z2 * r2 + z2 * r1) * cross / 24.0;
    }
    
    return {std::abs(Izz_area), std::abs(Irr_area), std::abs(Izr_area)};
}

std::tuple<double, double, double> FinSet::areaSecondMomentsAboutCentroid() const {
    auto [Izz_o, Irr_o, Izr_o] = areaSecondMomentsAboutOrigin();
    double A = planformArea();
    auto c = centroid();
    double cz = c.x();
    double cr = c.y();
    
    double Izz_c = Izz_o - A * cr * cr;
    double Irr_c = Irr_o - A * cz * cz;
    double Izr_c = Izr_o - A * cz * cr;
    
    return {Izz_c, Irr_c, Izr_c};
}

double FinSet::singleFinMass() const {
    return planformArea() * surfaceDensity();
}

Eigen::Matrix3d FinSet::singleFinInertiaLocal() const {
    double m = singleFinMass();
    auto [Izz_area, Irr_area, Izr_area] = areaSecondMomentsAboutCentroid();
    
    double I_z_from_planform = surfaceDensity() * Izz_area;
    double I_r_from_planform = surfaceDensity() * Irr_area;
    
    double t = thickness_;
    
    double Ixx = I_r_from_planform + m * t * t / 12.0;
    double Iyy = I_z_from_planform + I_r_from_planform;
    double Izz = I_z_from_planform + m * t * t / 12.0;
    
    Eigen::Matrix3d I = Eigen::Matrix3d::Zero();
    I(0, 0) = Ixx;
    I(1, 1) = Iyy;
    I(2, 2) = Izz;
    return I;
}

MassProperties FinSet::rawMassProperties() const {
    MassProperties props;
    double m_single = singleFinMass();
    props.mass = n_fins_ * m_single;
    
    auto c = centroid();
    double cz = c.x();
    double cr = c.y();
    
    props.cgLocal = Eigen::Vector3d(0.0, 0.0, cz);
    
    if (props.mass <= 0.0) return props;
    
    double d = getReferenceOuterRadius() + cr;
    Eigen::Matrix3d I_single = singleFinInertiaLocal();
    Eigen::Matrix3d I_total = Eigen::Matrix3d::Zero();
    
    const double pi = std::acos(-1.0);
    for (int i = 0; i < n_fins_; ++i) {
        double theta = 2.0 * pi * i / n_fins_;
        double c_theta = std::cos(theta);
        double s_theta = std::sin(theta);
        
        Eigen::Matrix3d R;
        R << c_theta, -s_theta, 0.0,
             s_theta,  c_theta, 0.0,
             0.0,      0.0,     1.0;
             
        Eigen::Matrix3d I_rot = R * I_single * R.transpose();
        
        Eigen::Vector3d r_vec(d * c_theta, d * s_theta, 0.0);
        Eigen::Matrix3d I_pa = m_single * (r_vec.squaredNorm() * Eigen::Matrix3d::Identity() - r_vec * r_vec.transpose());
        
        I_total += I_rot + I_pa;
    }
    
    props.inertiaLocal = I_total;
    return props;
}

AeroProperties FinSet::rawAeroProperties(const FlightCondition& fc) const {
    // Default implementation. TrapezoidalFinSet will override this.
    return AeroProperties{};
}

double FinSet::axialLength() const {
    auto pts = planformPoints();
    if (pts.empty()) return 0.0;
    double min_z = pts[0].x();
    double max_z = pts[0].x();
    for (const auto& p : pts) {
        if (p.x() < min_z) min_z = p.x();
        if (p.x() > max_z) max_z = p.x();
    }
    return max_z - min_z;
}

DragBreakdown FinSet::dragBreakdown(const FlightCondition& fc) const {
    DragBreakdown drag;

    const double sref = DragModels::safeReferenceArea(fc);
    if (sref <= 0.0) {
        return drag;
    }

    const double area = planformArea();
    const double chord = std::max(axialLength(), 1e-9);
    const double exposedWettedAreaPerFin = 2.0 * area;
    const double re = DragModels::reynoldsForLength(fc, chord);
    const double thicknessRatio = thickness() / chord;

    const double frictionPerFin = 1.05 * DragModels::skinFrictionCf(fc.mach, re) * exposedWettedAreaPerFin / sref;
    const double pressurePerFin = area / sref * (0.8 * thicknessRatio * thicknessRatio + 2.1 * fc.alpha * fc.alpha);

    drag.friction = frictionPerFin * numFins();
    drag.pressure = pressurePerFin * numFins();
    drag.fin = pressurePerFin * numFins();
    drag.wave = DragModels::toGlobalCd(
        DragModels::waveDrag(fc.mach, std::max(0.05, thicknessRatio)),
        numFins() * area,
        sref
    );
    return drag;
}

// TrapezoidalFinSet Implementation

TrapezoidalFinSet::TrapezoidalFinSet(
    std::string name,
    int n_fins,
    double thickness,
    const Malzeme& material,
    double root_chord,
    double tip_chord,
    double span,
    double sweep_length,
    std::string cross_section
) : FinSet(std::move(name), n_fins, thickness, material, std::move(cross_section)),
    root_chord_(root_chord),
    tip_chord_(tip_chord),
    span_(span),
    sweep_length_(sweep_length)
{
}

std::vector<Eigen::Vector2d> TrapezoidalFinSet::planformPoints() const {
    return {
        Eigen::Vector2d(0.0, 0.0),
        Eigen::Vector2d(root_chord_, 0.0),
        Eigen::Vector2d(sweep_length_ + tip_chord_, span_),
        Eigen::Vector2d(sweep_length_, span_)
    };
}

AeroProperties TrapezoidalFinSet::rawAeroProperties(const FlightCondition& fc) const {
    // We need FinAero to compute our aerodynamic properties
    FinAero aero(
        0.0, // xForward: We compute aero properties relative to FinSet's origin!
        sweep_length_,
        root_chord_,
        tip_chord_,
        span_,
        getReferenceOuterRadius(),
        numFins()
    );
    
    return aero.properties(fc);
}

// EllipticalFinSet Implementation

EllipticalFinSet::EllipticalFinSet(
    std::string name,
    int n_fins,
    double thickness,
    const Malzeme& material,
    double root_chord,
    double span,
    std::string cross_section
) : FinSet(std::move(name), n_fins, thickness, material, std::move(cross_section)),
    root_chord_(root_chord),
    span_(span)
{
    if (root_chord_ <= 0.0) {
        throw std::invalid_argument("Root chord must be positive.");
    }
    if (span_ <= 0.0) {
        throw std::invalid_argument("Span must be positive.");
    }
}

EllipticalFinSet::EllipticalFinSet(
    std::string name,
    int n_fins,
    double thickness,
    const Malzeme& material,
    double root_chord,
    double /*tip_chord*/,
    double span,
    double /*sweep_length*/,
    std::string cross_section
) : EllipticalFinSet(
        std::move(name),
        n_fins,
        thickness,
        material,
        root_chord,
        span,
        std::move(cross_section)
    )
{
}

std::vector<Eigen::Vector2d> EllipticalFinSet::planformPoints() const {
    constexpr int curveSegments = 96;

    std::vector<Eigen::Vector2d> points;
    points.reserve(curveSegments + 2);
    points.emplace_back(0.0, 0.0);
    points.emplace_back(root_chord_, 0.0);

    const double halfChord = root_chord_ * 0.5;
    if (halfChord <= 0.0 || span_ <= 0.0) {
        return points;
    }

    for (int i = 1; i < curveSegments; ++i) {
        const double u = static_cast<double>(i) / curveSegments;
        const double z = root_chord_ * (1.0 - u);
        const double normalized = (z - halfChord) / halfChord;
        const double r = span_ * std::sqrt(std::max(0.0, 1.0 - normalized * normalized));
        points.emplace_back(z, r);
    }

    return points;
}

AeroProperties EllipticalFinSet::rawAeroProperties(const FlightCondition& fc) const {
    const double equivalentTipChord = root_chord_ * ((std::acos(-1.0) / 2.0) - 1.0);
    const double equivalentSweepLength = (root_chord_ - equivalentTipChord) * 0.5;

    FinAero aero(
        0.0,
        equivalentSweepLength,
        root_chord_,
        equivalentTipChord,
        span_,
        getReferenceOuterRadius(),
        numFins()
    );

    return aero.properties(fc);
}
