#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"
#include "freeform_planform.hpp"

namespace barrowman_drag {

using json = nlohmann::json;
constexpr double pi = 3.14159265358979323846;

struct SymmetricComponent {
    std::string id;
    std::string parentId;
    double position = 0.0;
    std::string shape;
    double length = 0.0;
    double foreRadius = 0.0;
    double aftRadius = 0.0;
    double shapeParameter = 0.0;
    double sinPhi = 0.0;
    double wettedArea = 0.0;
    double planformArea = 0.0;
};

struct FinComponent {
    std::string id;
    int count = 0;
    double area = 0.0;
    double span = 0.0;
    double thickness = 0.0;
    double macLength = 0.0;
    double cosLeadingSweep = 1.0;
    std::string crossSection = "square";
    double wettedArea = 0.0;
    double planformArea = 0.0;
};

struct TubeComponent {
    std::string id;
    double length = 0.0;
    double innerRadius = 0.0;
    double outerRadius = 0.0;
    double roughness = 2.0e-6;
    int count = 1;
    double wettedArea = 0.0;
    double planformArea = 0.0;
};

struct RailButtonComponent {
    std::string id;
    int count = 1;
    double height = 0.0155;
    double outerDiameter = 0.012;
    double innerDiameter = 0.008;
    double innerHeight = 0.005;
    double position = 0.0;
    double wettedArea = 0.0;
    double planformArea = 0.0;
};

struct TubeFinComponent {
    std::string id;
    int count = 1;
    double length = 0.0;
    double innerRadius = 0.0;
    double outerRadius = 0.0;
    double bodyRadius = 0.0;
    double wettedArea = 0.0;
    double intersticeArea = 0.0;
    double planformArea = 0.0;
};

struct Model {
    double referenceArea = 0.0;
    double aerodynamicLength = 0.0;
    double bodyWettedArea = 0.0;
    double otherWettedArea = 0.0;
    double pressureArea = 0.0;
    double baseArea = 0.0;
    double maxBodyRadius = 0.0;
    double bodyMinX = 0.0;
    double bodyMaxX = 0.0;
    double roughness = 2.0e-6;
    bool perfectFinish = false;
    double bodyLiftPlanformArea = 0.0;
    double bodyLiftPlanformMoment = 0.0;
    std::vector<SymmetricComponent> symmetricComponents;
    std::vector<FinComponent> finComponents;
    std::vector<TubeComponent> tubeComponents;
    std::vector<RailButtonComponent> railButtonComponents;
    std::vector<TubeFinComponent> tubeFinComponents;
};

struct ComponentDrag {
    double frictionCd = 0.0;
    double pressureCd = 0.0;
    double baseCd = 0.0;
    double totalCd = 0.0;
};

struct Result {
    double reynolds = 0.0;
    double skinFriction = 0.0;
    double frictionCd = 0.0;
    double pressureCd = 0.0;
    double baseCd = 0.0;
    double totalCd = 0.0;
    double axialCd = 0.0;
    std::map<std::string, ComponentDrag> components;
};

inline double number(
    const json& object,
    const char* key,
    double fallback = 0.0
) {
    if (!object.is_object() || !object.contains(key) || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<double>();
}

inline std::string text(
    const json& object,
    const char* key,
    const std::string& fallback = ""
) {
    if (!object.is_object() || !object.contains(key) || object[key].is_null()) {
        return fallback;
    }
    return object[key].get<std::string>();
}

inline double stagnationCd(double mach) {
    const double m2 = mach * mach;
    double pressure;
    if (mach <= 1.0) {
        pressure = 1.0 + m2 / 4.0 + m2 * m2 / 40.0;
    } else {
        const double m4 = m2 * m2;
        const double m6 = m4 * m2;
        pressure = 1.84 - 0.76 / m2 + 0.166 / m4 + 0.035 / m6;
    }
    return 0.85 * pressure;
}

inline double baseCoefficient(double mach) {
    return mach <= 1.0
        ? 0.12 + 0.13 * mach * mach
        : 0.25 / std::max(mach, 1e-9);
}

inline double interpolate(double x, const std::vector<double>& xs,
                          const std::vector<double>& ys) {
    if (xs.empty() || ys.empty()) return 0.0;
    if (x <= xs.front()) return ys.front();
    if (x >= xs.back()) return ys.back();
    const auto upper = std::upper_bound(xs.begin(), xs.end(), x);
    const std::size_t i = static_cast<std::size_t>(upper - xs.begin() - 1);
    const double f = (x - xs[i]) / (xs[i + 1] - xs[i]);
    return ys[i] + f * (ys[i + 1] - ys[i]);
}

inline double hermite(double x, double x0, double x1, double y0, double y1,
                      double derivative0, double derivative1) {
    const double width = x1 - x0;
    const double u = std::clamp((x - x0) / width, 0.0, 1.0);
    const double h00 = 2 * u * u * u - 3 * u * u + 1;
    const double h10 = u * u * u - 2 * u * u + u;
    const double h01 = -2 * u * u * u + 3 * u * u;
    const double h11 = u * u * u - u * u;
    return h00 * y0 + h10 * width * derivative0
        + h01 * y1 + h11 * width * derivative1;
}

inline double tableValue(double mach, const std::vector<double>& xs,
                         const std::vector<double>& ys) {
    if (xs.empty()) return 0.0;
    if (mach < xs.front() && ys.front() < 0.001) return 0.0;
    return interpolate(mach, xs, ys);
}

inline double ogivePressureCd(double mach, double parameter, double sinPhi) {
    const double s = std::max(sinPhi, 0.0);
    const double multiplier = 0.72 * (parameter - 0.5) * (parameter - 0.5) + 0.82;
    const double atOne = s;
    const double atOneThree = 2.1 * s * s + 0.6019 * s;
    const double derivativeAtOne = 4.0 / 2.4 * (1.0 - 0.5 * atOne);
    const auto transonic = [&](double m) {
        return multiplier * hermite(m, 1.0, 1.3, atOne, atOneThree,
                                    derivativeAtOne, -1.1341 * s);
    };
    if (mach >= 1.3) {
        return std::max(multiplier * (2.1 * s * s
            + 0.5 * s / std::sqrt(std::max(mach * mach - 1.0, 1e-12))), 0.0);
    }
    if (mach >= 1.0) return std::max(transonic(mach), 0.0);

    // Port SymmetricComponentCalc.calculateNoseInterpolator exactly.  Its
    // subsonic section is a power law joined to the M=1.0 interpolator,
    // rather than a Hermite curve from Mach zero.  The source gets the slope
    // from its first 0.02-Mach linear-interpolator segment.
    const double atOneValue = multiplier * atOne;
    if (atOneValue < 0.001) return 0.0;
    const double cdMachZero = 0.8 * s * s;
    const double minDerivative = (transonic(1.02) - atOneValue) / 0.02;
    if (cdMachZero >= atOneValue - 0.01 || minDerivative <= 0.01) {
        return std::max(atOneValue, 0.0);
    }
    const double exponent = minDerivative / (atOneValue - cdMachZero);
    return std::max((atOneValue - cdMachZero) * std::pow(
        std::max(mach, 0.0), exponent) + cdMachZero, 0.0);
}

inline double empiricalPowerCd(double parameter, double mach) {
    static const std::vector<double> x14X={1.2,1.3,1.4,1.6,1.8,2.2,2.6,3.0,3.6};
    static const std::vector<double> x14Y={.140,.156,.169,.192,.206,.227,.241,.249,.252};
    static const std::vector<double> x12X={.925,.95,1.,1.05,1.1,1.2,1.3,1.7,2.};
    static const std::vector<double> x12Y={0,.014,.050,.060,.059,.081,.084,.085,.078};
    static const std::vector<double> x34X={.8,.9,1.,1.06,1.2,1.4,1.6,2.,2.8,3.4};
    static const std::vector<double> x34Y={0,.015,.078,.121,.110,.098,.090,.084,.078,.074};
    const auto blend=[](double a,double b,double f){return a+std::clamp(f,0.,1.)*(b-a);};
    if(parameter<=.25) return blend(stagnationCd(mach),tableValue(mach,x14X,x14Y),parameter*4);
    if(parameter<=.5) return blend(tableValue(mach,x14X,x14Y),tableValue(mach,x12X,x12Y),(parameter-.25)*4);
    return blend(tableValue(mach,x12X,x12Y),tableValue(mach,x34X,x34Y),(parameter-.5)*4);
}

inline double empiricalParabolicCd(double parameter, double mach) {
    static const std::vector<double> pX={.95,.975,1.,1.05,1.1,1.2,1.4,1.7};
    static const std::vector<double> pY={0,.016,.041,.092,.109,.119,.113,.108};
    static const std::vector<double> p12X={.8,.9,.95,1.,1.05,1.1,1.3,1.5,1.8};
    static const std::vector<double> p12Y={0,.016,.042,.100,.126,.125,.100,.090,.088};
    static const std::vector<double> p34X={.9,.95,1.,1.05,1.1,1.2,1.4,1.7};
    static const std::vector<double> p34Y={0,.023,.073,.098,.107,.106,.089,.082};
    const auto blend=[](double a,double b,double f){return a+std::clamp(f,0.,1.)*(b-a);};
    if(parameter<=.5) return blend(0.,tableValue(mach,p12X,p12Y),parameter*2);
    if(parameter<=.75) return blend(tableValue(mach,p12X,p12Y),tableValue(mach,p34X,p34Y),(parameter-.5)*4);
    return blend(tableValue(mach,p34X,p34Y),tableValue(mach,pX,pY),(parameter-.75)*4);
}

inline double empiricalNoseCd(const std::string& shape,double parameter,double mach) {
    static const std::vector<double> eX={1.2,1.25,1.3,1.4,1.6,2.,2.4};
    static const std::vector<double> eY={.110,.128,.140,.148,.152,.159,.162};
    static const std::vector<double> vkX={.9,.95,1.,1.05,1.1,1.2,1.4,1.6,2.,3.};
    static const std::vector<double> vkY={0,.010,.027,.055,.070,.081,.095,.097,.091,.083};
    static const std::vector<double> lvX={.9,.95,1.,1.05,1.1,1.2,1.4,1.6,2.};
    static const std::vector<double> lvY={0,.010,.024,.066,.084,.100,.114,.117,.113};
    if(shape=="Tangent Ogive"||shape=="Conical") return -1.;
    if(shape=="Power") return empiricalPowerCd(parameter,mach);
    if(shape=="Parabolic") return empiricalParabolicCd(parameter,mach);
    if(shape=="Haack") {
        const double a=tableValue(mach,vkX,vkY),b=tableValue(mach,lvX,lvY);
        return a+std::clamp(parameter*3.,0.,1.)*(b-a);
    }
    return tableValue(mach,eX,eY);
}

inline double symmetricPressureCd(const SymmetricComponent& c,double mach,double refArea) {
    const double frontal=pi*std::abs(c.foreRadius*c.foreRadius-c.aftRadius*c.aftRadius);
    if(!(frontal>0)||!(refArea>0)) return 0;
    if(c.length<.001) return (c.foreRadius<c.aftRadius?stagnationCd(mach):baseCoefficient(mach))*frontal/refArea;
    if(c.aftRadius<c.foreRadius) {
        const double f=c.length/std::max(2*std::abs(c.aftRadius-c.foreRadius),1e-12);
        if(f>=3) return 0;
        return baseCoefficient(mach)*frontal/refArea*(f<=1?1:(3-f)/2);
    }
    double local=empiricalNoseCd(c.shape,c.shapeParameter,mach);
    if(local<0) local=ogivePressureCd(mach,c.shape=="Conical"?0.:c.shapeParameter,c.sinPhi);
    if (c.shape != "Conical" && c.shape != "Tangent Ogive") {
        const double f=c.length/std::max(2*std::abs(c.aftRadius-c.foreRadius),1e-12);
        const double exponent=std::log(std::max(f+1,1.))/std::log(4.);
        const double stag=std::max(stagnationCd(mach),1e-12);
        local=stag*std::pow(std::max(local/stag,0.),exponent);
    }
    return local*frontal/refArea;
}

inline double finRoundedPressureCd(double mach) {
    if(mach<.9) return std::pow(std::max(1-mach*mach,1e-12),-.417)-1;
    if(mach<1) return 1-1.785*(mach-.9);
    const double m2=mach*mach;
    return 1.214-.502/m2+.1095/(m2*m2);
}

inline double tubePressureCd(const TubeComponent& t,double mach,double modelRe,
                             double aeroLength,double refArea) {
    const double inner=pi*t.innerRadius*t.innerRadius,total=pi*t.outerRadius*t.outerRadius;
    const double diameter=2*t.innerRadius,frontal=std::max(total-inner,0.);
    double internal=0;
    if(diameter>1e-12&&inner>1e-12&&aeroLength>1e-12) {
        const double re=std::max(modelRe*diameter/aeroLength,1.);
        const double term=t.roughness/(3.7*diameter)+5.74/std::pow(re,.9);
        const double den=std::log10(std::max(term,1e-12));
        internal=.25/(den*den)*t.length/diameter;
    }
    return t.count*(internal*inner+.7*(stagnationCd(mach)+baseCoefficient(mach))*frontal)/std::max(refArea,1e-12);
}

inline double railButtonDragCoefficient(double mach) {
    static const std::vector<double> cdDomain = {0.0, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 1.0, 1.6, 2.0, 2.8, 100.0};
    static const std::vector<double> cdRange = {1.2, 1.22, 1.25, 1.3, 1.4, 1.5, 1.6, 2.1, 1.5, 1.45, 1.33, 1.33};
    return interpolate(mach, cdDomain, cdRange);
}

inline double railButtonPressureCd(
    const RailButtonComponent& button,
    double mach,
    double modelRe,
    double aeroLength,
    double refArea
) {
    if (button.count <= 0 || !(refArea > 0.0)) return 0.0;
    const double outerArea = button.height * button.outerDiameter;
    const double notchArea = std::max((button.outerDiameter - button.innerDiameter) * button.innerHeight, 0.0);
    const double buttonRefArea = std::max(outerArea - notchArea, 0.0);
    if (!(buttonRefArea > 0.0)) return 0.0;

    double cdSum = 0.0;
    if (mach > 1e-9 && aeroLength > 1e-9 && modelRe > 1e-9) {
        const double x = std::max(button.position, 0.001);
        const double rex = modelRe * (x / aeroLength);
        const double del = 0.37 * x / std::pow(std::max(rex, 1.0), 0.2);
        
        double localMach = mach;
        if (button.height > del) {
            localMach = (button.height - 0.5 * del) * mach / std::max(button.height, 1e-9);
        } else if (del > 1e-9) {
            localMach = (button.height / 2.0) / del * mach;
        }
        
        double cd = railButtonDragCoefficient(localMach);
        cd = cd * (localMach * localMach) / std::max(mach * mach, 1e-12);
        cdSum = cd;
    } else {
        cdSum = 8.786395e-4;
    }

    return button.count * cdSum * stagnationCd(mach) * buttonRefArea / std::max(refArea, 1e-12);
}

inline double tubeFinPressureCd(
    const TubeFinComponent& tf,
    double mach,
    double modelRe,
    double aeroLength,
    double roughness,
    double refArea
) {
    if (tf.count <= 0 || !(refArea > 0.0)) return 0.0;

    // Exact TubeFinSetCalc composition from OpenRocket:
    //   TubeCalc pressure loss through one hollow tube
    //   + TubeCalc's 0.7 stagnation/base annular frontal term
    //   + TubeFinSetCalc's exposed body/tube interstice term.
    // The source calculator is invoked per physical tube, hence ``count``
    // multiplies every one-tube contribution.
    const double innerArea = pi * tf.innerRadius * tf.innerRadius;
    const double outerArea = pi * tf.outerRadius * tf.outerRadius;
    const double frontalArea = std::max(outerArea - innerArea, 0.0);
    const double diameter = 2.0 * tf.innerRadius;
    double internalCd = 0.0;
    if (diameter > 1e-12 && innerArea > 1e-12 && aeroLength > 1e-12) {
        const double re = std::max(modelRe * diameter / aeroLength, 1.0);
        const double term = roughness / (3.7 * diameter)
            + 5.74 / std::pow(re, 0.9);
        const double denominator = std::log10(std::max(term, 1e-12));
        const double darcyFriction = 0.25 / (denominator * denominator);
        internalCd = darcyFriction * tf.length / diameter;
    }
    const double pressure = internalCd * innerArea
        + 0.7 * (stagnationCd(mach) + baseCoefficient(mach)) * frontalArea
        + (stagnationCd(mach) + baseCoefficient(mach)) * tf.intersticeArea;
    return static_cast<double>(tf.count) * pressure / std::max(refArea, 1e-12);
}

inline double roughnessCorrection(double mach) {
    if (mach < 0.9) return 1.0 - 0.1 * mach * mach;
    if (mach > 1.1) return 1.0 / (1.0 + 0.18 * mach * mach);
    const double c1 = 1.0 - 0.1 * 0.9 * 0.9;
    const double c2 = 1.0 / (1.0 + 0.18 * 1.1 * 1.1);
    return c2 * (mach - 0.9) / 0.2 + c1 * (1.1 - mach) / 0.2;
}

inline double frictionCoefficient(
    double mach,
    double reynolds,
    bool perfectFinish
) {
    if (!(reynolds > 0.0)) return 0.0;
    const double re = std::max(reynolds, 1.0);
    double cf;
    double c1 = 1.0;
    double c2 = 1.0;

    if (perfectFinish) {
        if (re < 1.0e4) {
            cf = 1.33e-2;
        } else if (re < 5.39e5) {
            cf = 1.328 / std::sqrt(re);
        } else {
            const double denominator = 1.50 * std::log(re) - 5.6;
            cf = 1.0 / (denominator * denominator) - 1700.0 / re;
        }
        if (mach < 1.1 && re > 1.0e6) {
            c1 = re < 3.0e6
                ? 1.0 - 0.1 * mach * mach * (re - 1.0e6) / 2.0e6
                : 1.0 - 0.1 * mach * mach;
        }
        if (mach > 0.9 && re > 1.0e6) {
            const double compressible =
                1.0 / std::pow(1.0 + 0.045 * mach * mach, 0.25);
            c2 = re < 3.0e6
                ? 1.0 + (compressible - 1.0) * (re - 1.0e6) / 2.0e6
                : compressible;
        }
    } else {
        if (re < 1.0e4) {
            cf = 1.48e-2;
        } else {
            const double denominator = 1.50 * std::log(re) - 5.6;
            cf = 1.0 / (denominator * denominator);
        }
        if (mach < 1.1) c1 = 1.0 - 0.1 * mach * mach;
        if (mach > 0.9) {
            c2 = 1.0 / std::pow(1.0 + 0.15 * mach * mach, 0.58);
        }
    }

    if (mach < 0.9) return cf * c1;
    if (mach < 1.1) {
        return cf * (
            c2 * (mach - 0.9) / 0.2 + c1 * (1.1 - mach) / 0.2);
    }
    return cf * c2;
}

inline double axialMultiplier(double angleOfAttack) {
    double aoa = std::clamp(angleOfAttack, 0.0, pi);
    const bool reverse = aoa >= pi / 2.0;
    if (reverse) aoa = pi - aoa;
    const double split = 17.0 * pi / 180.0;
    double multiplier;
    if (aoa < split) {
        const double u = aoa / split;
        multiplier = 1.0 + 0.3 * (3.0 * u * u - 2.0 * u * u * u);
    } else {
        const double u = (aoa - split) / (pi / 2.0 - split);
        const double smooth =
            1.0 - 10.0 * std::pow(u, 3.0)
                + 15.0 * std::pow(u, 4.0)
                - 6.0 * std::pow(u, 5.0);
        multiplier = 1.3 * std::max(smooth, 0.0);
    }
    return reverse ? -multiplier : multiplier;
}

inline double noseRadius(
    const std::string& shape,
    double z,
    double length,
    double radius,
    const json& params
) {
    if (!(length > 0.0)) return 0.0;
    const double x = std::clamp(z / length, 0.0, 1.0);
    if (shape == "Conical") return radius * x;
    if (shape == "Parabolic") {
        const double k = number(params, "K", 0.5);
        return radius * (2.0 * x - k * x * x) / std::max(2.0 - k, 1e-9);
    }
    if (shape == "Power") {
        return radius * std::pow(x, number(params, "n", 0.5));
    }
    if (shape == "Haack") {
        const double theta =
            std::acos(std::clamp(1.0 - 2.0 * x, -1.0, 1.0));
        const double c = number(params, "C", 0.0);
        const double inside = std::max(
            theta - 0.5 * std::sin(2.0 * theta)
                + c * std::pow(std::sin(theta), 3.0),
            0.0);
        return radius * std::sqrt(inside / pi);
    }
    if (!(radius > 0.0)) return 0.0;
    const double rho = (radius * radius + length * length) / (2.0 * radius);
    return std::sqrt(std::max(
        rho * rho - (length - z) * (length - z), 0.0))
        + radius - rho;
}

inline double noseWettedArea(
    const std::string& shape,
    double length,
    double radius,
    const json& params
) {
    constexpr int slices = 400;
    if (!(length > 0.0) || !(radius > 0.0)) return 0.0;
    double area = 0.0;
    double previousZ = 0.0;
    double previousR = noseRadius(shape, 0.0, length, radius, params);
    for (int i = 1; i <= slices; ++i) {
        const double z = length * i / slices;
        const double r = noseRadius(shape, z, length, radius, params);
        const double dz = z - previousZ;
        const double dr = r - previousR;
        area += pi * (r + previousR) * std::sqrt(dz * dz + dr * dr);
        previousZ = z;
        previousR = r;
    }
    return area;
}

inline std::pair<double,double> nosePlanform(
    const std::string& shape,double length,double radius,const json& params) {
    constexpr int slices=400;
    if(!(length>0)||!(radius>0)) return {0,0};
    double area=0,moment=0;
    const double dz=length/slices;
    for(int i=0;i<slices;++i) {
        const double z=(i+.5)*dz;
        const double strip=2*noseRadius(shape,z,length,radius,params)*dz;
        area+=strip;
        moment+=strip*z;
    }
    return {area, area>0?moment/area:0};
}

inline double nosePressureFactor(
    const std::string& shape,
    double length,
    double radius
) {
    const double fineness = length / std::max(2.0 * radius, 1e-9);
    double shapeFactor = 0.12;
    if (shape == "Conical") shapeFactor = 0.16;
    else if (shape == "Parabolic" || shape == "Haack") shapeFactor = 0.10;
    else if (shape == "Power") shapeFactor = 0.14;
    return shapeFactor / std::max(std::sqrt(fineness), 1.0);
}

inline Model build(const json& input, double referenceDiameter) {
    Model model;
    model.referenceArea = pi * referenceDiameter * referenceDiameter / 4.0;
    if (input.contains("drag_settings")) {
        const json& settings = input["drag_settings"];
        model.roughness = number(settings, "roughness_m", model.roughness);
        model.perfectFinish =
            settings.value("perfect_finish", model.perfectFinish);
    }
    if (!input.contains("components") || !input["components"].is_array()) {
        return model;
    }

    double currentZ = 0.0;
    double bodyMin = 1.0e100;
    double bodyMax = 0.0;
    double previousAftRadius = 0.0;

    for (const auto& component : input["components"]) {
        // Internal parts remain in mass properties but create no native
        // wetted, pressure, or drag term.
        if (!component.value("aero_active", true)
            || text(component, "aero_exposure", "external") == "internal") {
            continue;
        }
        const std::string type = text(component, "type");
        const std::string id = text(component, "id", "unknown");
        const std::string parentId = text(component, "parent_id");
        const bool reverseFromTail = component.value("resolved_reverse_from_tail", false)
            || text(component, "pos_mode") == "Reverse from Tail"
            || component.value("params", json::object()).value("is_flipped", false);
        if (type == "Stage Separator") continue;
        const json params = component.contains("params")
            ? component["params"] : json::object();
        const std::string mode =
            text(component, "pos_mode", "Automatic (Stacked)");
        double position = number(component, "pos_value", 0.0);
        if (mode == "Automatic (Stacked)") position = currentZ;
        else if (mode == "Relative Offset") position = currentZ + position;

        double length = 0.0;
        bool symmetric = false;
        double foreRadius = 0.0;
        double aftRadius = 0.0;
        if (type == "Nose Cone") {
            symmetric = true;
            length = number(params, "length", 0.3);
            const double noseBaseRadius =
                number(params, "diameter", referenceDiameter) / 2.0;
            aftRadius = noseBaseRadius;
            if (reverseFromTail) std::swap(foreRadius, aftRadius);
            const std::string shape =
                text(component, "sub_type", "Tangent Ogive");
            const double wetted = noseWettedArea(
                shape, length, noseBaseRadius, params);
            model.bodyWettedArea += wetted;
            const auto planform = nosePlanform(
                shape, length, noseBaseRadius, params);
            model.bodyLiftPlanformArea += planform.first;
            model.bodyLiftPlanformMoment += planform.first
                * (position + planform.second);
            double shapeParameter = 0.0;
            if (shape == "Tangent Ogive") shapeParameter = 1.0;
            else if (shape == "Parabolic") shapeParameter = number(params, "K", 0.5);
            else if (shape == "Power") shapeParameter = number(params, "n", 0.5);
            else if (shape == "Haack") shapeParameter = number(params, "C", 0.0);
            double sinPhi = 0.0;
            if (shape != "Tangent Ogive" && length > 0.0) {
                if (shape == "Conical") {
                    sinPhi = noseBaseRadius / std::max(
                        std::hypot(noseBaseRadius, length), 1e-12);
                } else {
                    const double nearRadius = noseRadius(
                        shape, 0.99 * length, length, noseBaseRadius, params);
                    sinPhi = (noseBaseRadius - nearRadius) / std::max(
                        std::hypot(noseBaseRadius - nearRadius, 0.01 * length), 1e-12);
                }
            }
            model.symmetricComponents.push_back({
                id, parentId, position, shape, length, 0.0, aftRadius,
                shapeParameter, sinPhi, wetted, planform.first});
        } else if (type == "Body Tube") {
            symmetric = true;
            length = number(params, "length", 1.0);
            foreRadius = aftRadius =
                number(params, "diameter", referenceDiameter) / 2.0;
            const double wetted = 2.0 * pi * aftRadius * length;
            model.bodyWettedArea += wetted;
            const double planform = 2.0 * aftRadius * length;
            model.bodyLiftPlanformArea += planform;
            model.bodyLiftPlanformMoment += planform
                * (position + 0.5 * length);
            model.symmetricComponents.push_back({
                id, parentId, position, "Cylinder", length, foreRadius, aftRadius,
                0.0, 0.0, wetted, planform});
        } else if (type == "Transition") {
            symmetric = true;
            length = number(params, "length", 0.1);
            foreRadius =
                number(params, "fore_diameter", referenceDiameter) / 2.0;
            aftRadius = number(
                params, "aft_diameter", referenceDiameter * 0.8) / 2.0;
            if (reverseFromTail) std::swap(foreRadius, aftRadius);
            const double slant = std::hypot(length, aftRadius - foreRadius);
            const double wetted = pi * (foreRadius + aftRadius) * slant;
            model.bodyWettedArea += wetted;
            const double planform = (foreRadius + aftRadius) * length;
            const double center = foreRadius + aftRadius > 1e-12
                ? length * (foreRadius + 2.0*aftRadius)
                    / (3.0*(foreRadius + aftRadius)) : 0.5*length;
            model.bodyLiftPlanformArea += planform;
            model.bodyLiftPlanformMoment += planform * (position + center);
            const double sinPhi = length > 0.0
                ? std::abs(aftRadius - foreRadius) / slant : 1.0;
            model.symmetricComponents.push_back({
                id, parentId, position, "Conical", length, foreRadius, aftRadius,
                0.0, sinPhi, wetted, planform});
        } else if (type == "Fins" || type == "Freeform Fin Set") {
            const bool isFreeform = type == "Freeform Fin Set";
            const double count =
                number(params, "n_fins", number(params, "n", 3.0));
            const double root = number(params, "root_chord", 0.25);
            const double tip = number(params, "tip_chord", 0.1);
            const double parameterSpan = number(params, "span", 0.12);
            const double sweep = number(params, "sweep_length", 0.1);
            const double thickness = number(params, "thickness", 0.003);
            FinPlanform::Metrics freeform;
            if (isFreeform) {
                freeform = freeform_planform::metricsFromParams(params);
            }
            const double span = isFreeform ? freeform.span : parameterSpan;
            const double planform = isFreeform
                ? freeform.area : 0.5 * (root + tip) * span;
            length = isFreeform
                ? freeform.axialLength : std::max(root, sweep + tip);
            const double mac = isFreeform
                ? freeform.macLength
                : (root + tip > 1e-12
                    ? 2.0 * (root * root + root * tip + tip * tip)
                        / (3.0 * (root + tip)) : 0.0);
            const double cosLead = isFreeform
                ? freeform.leadingEdgeCosine
                : span / std::max(std::hypot(span, sweep), 1e-12);
            const double wetted = 2.0 * count * planform
                * (1.0 + 2.0 * thickness / std::max(mac, 1e-12));
            model.otherWettedArea += wetted;
            model.finComponents.push_back({
                id, static_cast<int>(std::lround(count)), planform, span,
                thickness, mac, cosLead,
                text(params, "cross_section", "square"), wetted, planform});
        } else if (type == "Launch Lug") {
            length = number(params, "length", 0.05);
            const double outer =
                number(params, "outer_diameter", 0.016) / 2.0;
            const double inner =
                number(params, "inner_diameter", 0.012) / 2.0;
            const double wetted = 2.0 * pi * outer * length;
            model.tubeComponents.push_back({
                id, length, inner, outer, model.roughness, 1, wetted, wetted});
        } else if (type == "Rail Button") {
            const double count = number(params, "instance_count", number(params, "n", 1.0));
            length = number(params, "length", number(params, "height", 0.0155));
            const double height = number(params, "height", number(params, "total_height", 0.0155));
            const double outerDia = number(params, "outer_diameter", 0.012);
            const double innerDia = number(params, "inner_diameter", 0.008);
            const double innerHt = number(params, "inner_height", 0.005);
            const double outerArea = height * outerDia;
            const double notchArea = std::max((outerDia - innerDia) * innerHt, 0.0);
            const double refA = std::max(outerArea - notchArea, 0.0);
            const double wetted = 2.0 * count * refA;
            model.otherWettedArea += wetted;
            model.railButtonComponents.push_back({
                id, static_cast<int>(std::lround(count)), height, outerDia, innerDia, innerHt, position, wetted, refA});
        } else if (type == "Tube Fin Set") {
            // These are canonical importer names.  Retain old aliases only
            // for saved designs produced before TubeFinSet was introduced.
            const double count = number(params, "fin_count",
                number(params, "n_fins", number(params, "n", 3.0)));
            length = number(params, "length", 0.1);
            const double innerR = number(params, "inner_radius",
                number(params, "inner_diameter", 0.05) / 2.0);
            const double outerR = number(params, "outer_radius",
                number(params, "outer_diameter", 0.054) / 2.0);
            const double bodyR = number(params, "body_radius",
                previousAftRadius > 0.0 ? previousAftRadius : model.maxBodyRadius);
            
            double intersticeArea = 0.0;
            double wettedArea = 0.0;
            if (outerR > 0.0 && bodyR > 0.0) {
                const double sumR = bodyR + outerR;
                const double d = std::sqrt(std::max(sumR * sumR - outerR * outerR, 0.0));
                const double a = d * outerR;
                const double theta1 = std::acos(std::clamp(outerR / sumR, -1.0, 1.0));
                const double theta2 = pi / 2.0 - theta1;
                const double a1 = outerR * outerR * theta1;
                const double a2 = bodyR * bodyR * theta2;
                intersticeArea = std::max(a - a1 - a2, 0.0);
                const double outerArea = length * 2.0 * (pi - theta1) * outerR;
                const double maskedArea = length * 2.0 * theta2 * bodyR;
                wettedArea = std::max(outerArea - maskedArea, 0.0) * count;
            } else {
                wettedArea = 2.0 * pi * outerR * length * count;
            }
            model.otherWettedArea += wettedArea;
            model.tubeFinComponents.push_back({
                id, static_cast<int>(std::lround(count)), length, innerR, outerR, bodyR, wettedArea, intersticeArea, pi * outerR * outerR});
        }

        if (symmetric) {
            bodyMin = std::min(bodyMin, position);
            bodyMax = std::max(bodyMax, position + length);
            model.maxBodyRadius = std::max(
                model.maxBodyRadius, std::max(foreRadius, aftRadius));
            previousAftRadius = aftRadius;
        }
        model.aerodynamicLength =
            std::max(model.aerodynamicLength, position + length);
        if (mode == "Automatic (Stacked)" || mode == "Relative Offset") {
            currentZ = position + length;
        }
    }

    if (bodyMin < 1.0e99) {
        model.bodyMinX = bodyMin;
        model.bodyMaxX = bodyMax;
    }
    return model;
}

inline Result calculate(
    const Model& model,
    double mach,
    double reynolds,
    double angleOfAttack = 0.0
) {
    Result result;
    result.reynolds = reynolds;
    if (!(model.referenceArea > 0.0)) return result;

    const double baseCf =
        frictionCoefficient(mach, reynolds, model.perfectFinish);
    const double roughnessLimited =
        model.aerodynamicLength > 0.0 && model.roughness > 0.0
        ? 0.032 * std::pow(
            model.roughness / model.aerodynamicLength, 0.2)
            * roughnessCorrection(mach)
        : 0.0;
    result.skinFriction = model.perfectFinish
        ? (reynolds > 1.0e6 && roughnessLimited > baseCf
            ? roughnessLimited : baseCf)
        : std::max(baseCf, roughnessLimited);

    const double fineness =
        (model.bodyMaxX - model.bodyMinX + 0.0001)
        / std::max(model.maxBodyRadius, 1e-9);
    const double bodyCorrection =
        model.maxBodyRadius > 0.0 ? 1.0 + 1.0 / (2.0 * fineness) : 1.0;
    result.frictionCd = result.skinFriction
        * (model.otherWettedArea + bodyCorrection * model.bodyWettedArea)
        / model.referenceArea;
    
    // Symmetric components are linked through their physical parent chain.
    // A PodSet/ParallelStage produces several such chains after resolution;
    // a global "last body" loses both their shoulder pressure and their
    // exposed aft base areas.
    std::map<std::string, std::vector<std::size_t>> symmetricByParent;
    for (std::size_t i = 0; i < model.symmetricComponents.size(); ++i) {
        symmetricByParent[model.symmetricComponents[i].parentId].push_back(i);
    }
    std::vector<double> incomingPressure(model.symmetricComponents.size(), 0.0);
    std::vector<double> exposedBase(model.symmetricComponents.size(), 0.0);
    for (auto& entry : symmetricByParent) {
        auto& indices = entry.second;
        std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
            return model.symmetricComponents[a].position < model.symmetricComponents[b].position;
        });
        for (std::size_t order = 0; order < indices.size(); ++order) {
            const SymmetricComponent& current = model.symmetricComponents[indices[order]];
            const SymmetricComponent* previous = order > 0
                ? &model.symmetricComponents[indices[order - 1]] : nullptr;
            const SymmetricComponent* next = order + 1 < indices.size()
                ? &model.symmetricComponents[indices[order + 1]] : nullptr;
            const double previousAft = previous ? previous->aftRadius : 0.0;
            if (previous && current.foreRadius > previousAft) {
                incomingPressure[indices[order]] = stagnationCd(mach) * pi
                    * (current.foreRadius * current.foreRadius
                        - previousAft * previousAft) / model.referenceArea;
            }
            const double nextFore = next ? next->foreRadius : 0.0;
            if (current.aftRadius > nextFore) {
                exposedBase[indices[order]] = baseCoefficient(mach) * pi
                    * (current.aftRadius * current.aftRadius
                        - nextFore * nextFore) / model.referenceArea;
            }
        }
    }

    const double aoaMult = axialMultiplier(angleOfAttack);

    for (std::size_t index = 0; index < model.symmetricComponents.size(); ++index) {
        const auto& component = model.symmetricComponents[index];
        ComponentDrag cd;
        cd.frictionCd = result.skinFriction * component.wettedArea * bodyCorrection / model.referenceArea;
        cd.pressureCd = symmetricPressureCd(component, mach, model.referenceArea)
            + incomingPressure[index];
        cd.baseCd = exposedBase[index];
        cd.totalCd = (cd.frictionCd + cd.pressureCd + cd.baseCd) * aoaMult;
        result.pressureCd += cd.pressureCd;
        result.baseCd += cd.baseCd;
        result.components[component.id] = cd;
    }
    
    for (const auto& fin : model.finComponents) {
        if (!(fin.area > 0.0)) continue;
        ComponentDrag cd;
        cd.frictionCd = result.skinFriction * fin.wettedArea / model.referenceArea;
        
        const bool rounded = fin.crossSection == "rounded"
            || fin.crossSection == "airfoil"
            || fin.crossSection == "diamond"
            || fin.crossSection == "biconvex";
        const double local = rounded ? finRoundedPressureCd(mach) : stagnationCd(mach);
        cd.pressureCd = fin.count * local * fin.cosLeadingSweep * fin.cosLeadingSweep
            * fin.span * fin.thickness / model.referenceArea;
            
        double factor = 0.0;
        if (fin.crossSection == "square") factor = 1.0;
        else if (fin.crossSection == "rounded") factor = 0.5;
        cd.baseCd = fin.count * factor * baseCoefficient(mach)
            * fin.span * fin.thickness / model.referenceArea;
            
        cd.totalCd = (cd.frictionCd + cd.pressureCd + cd.baseCd) * aoaMult;
        result.pressureCd += cd.pressureCd;
        result.baseCd += cd.baseCd;
        result.components[fin.id] = cd;
    }
    
    for (const auto& tube : model.tubeComponents) {
        ComponentDrag cd;
        // OpenRocket LaunchLugCalc explicitly contributes no skin-friction
        // drag.  ``tubeComponents`` represents launch lugs in this model;
        // leaving their wetted area out of the rocket-level friction sum but
        // adding it to the component row made the breakdown non-additive.
        cd.frictionCd = 0.0;
        cd.pressureCd = tubePressureCd(tube, mach, reynolds, model.aerodynamicLength, model.referenceArea);
        cd.totalCd = (cd.frictionCd + cd.pressureCd + cd.baseCd) * aoaMult;
        result.pressureCd += cd.pressureCd;
        result.components[tube.id] = cd;
    }

    for (const auto& button : model.railButtonComponents) {
        ComponentDrag cd;
        cd.frictionCd = 0.0;
        cd.pressureCd = railButtonPressureCd(button, mach, reynolds, model.aerodynamicLength, model.referenceArea);
        cd.baseCd = 0.0;
        cd.totalCd = cd.pressureCd * aoaMult;
        result.pressureCd += cd.pressureCd;
        result.components[button.id] = cd;
    }

    for (const auto& tf : model.tubeFinComponents) {
        ComponentDrag cd;
        cd.frictionCd = result.skinFriction * tf.wettedArea / model.referenceArea;
        cd.pressureCd = tubeFinPressureCd(tf, mach, reynolds,
            model.aerodynamicLength, model.roughness, model.referenceArea);
        cd.baseCd = 0.0;
        cd.totalCd = (cd.frictionCd + cd.pressureCd) * aoaMult;
        result.pressureCd += cd.pressureCd;
        result.components[tf.id] = cd;
    }

    result.totalCd = (result.frictionCd + result.pressureCd + result.baseCd) * aoaMult;
    result.axialCd = result.totalCd;
    return result;
}

inline json toJson(const Model& model) {
    json symmetric = json::array();
    for (const auto& c : model.symmetricComponents) symmetric.push_back({
        {"id", c.id}, {"parent_id", c.parentId}, {"position_m", c.position},
        {"shape", c.shape}, {"length_m", c.length},
        {"fore_radius_m", c.foreRadius}, {"aft_radius_m", c.aftRadius},
        {"shape_parameter", c.shapeParameter}, {"sin_phi", c.sinPhi},
        {"wetted_area_m2", c.wettedArea}, {"planform_area_m2", c.planformArea}});
    json fins = json::array();
    for (const auto& c : model.finComponents) fins.push_back({
        {"id", c.id}, {"count", c.count}, {"area_m2", c.area}, {"span_m", c.span},
        {"thickness_m", c.thickness}, {"mac_length_m", c.macLength},
        {"cos_leading_sweep", c.cosLeadingSweep},
        {"cross_section", c.crossSection},
        {"wetted_area_m2", c.wettedArea}, {"planform_area_m2", c.planformArea}});
    json tubes = json::array();
    for (const auto& c : model.tubeComponents) tubes.push_back({
        {"id", c.id}, {"length_m", c.length}, {"inner_radius_m", c.innerRadius},
        {"outer_radius_m", c.outerRadius}, {"roughness_m", c.roughness},
        {"count", c.count},
        {"wetted_area_m2", c.wettedArea}, {"planform_area_m2", c.planformArea}});
    json buttons = json::array();
    for (const auto& c : model.railButtonComponents) buttons.push_back({
        {"id", c.id}, {"count", c.count}, {"height_m", c.height},
        {"outer_diameter_m", c.outerDiameter}, {"inner_diameter_m", c.innerDiameter},
        {"inner_height_m", c.innerHeight}, {"position_m", c.position},
        {"wetted_area_m2", c.wettedArea}, {"planform_area_m2", c.planformArea}});
    json tubeFins = json::array();
    for (const auto& c : model.tubeFinComponents) tubeFins.push_back({
        {"id", c.id}, {"count", c.count}, {"length_m", c.length},
        {"inner_radius_m", c.innerRadius}, {"outer_radius_m", c.outerRadius},
        {"body_radius_m", c.bodyRadius}, {"wetted_area_m2", c.wettedArea},
        {"interstice_area_m2", c.intersticeArea}, {"planform_area_m2", c.planformArea}});
    return {
        {"reference_area_m2", model.referenceArea},
        {"aerodynamic_length_m", model.aerodynamicLength},
        {"body_wetted_area_m2", model.bodyWettedArea},
        {"other_wetted_area_m2", model.otherWettedArea},
        {"pressure_area_m2", model.pressureArea},
        {"base_area_m2", model.baseArea},
        {"max_body_radius_m", model.maxBodyRadius},
        {"body_min_x_m", model.bodyMinX},
        {"body_max_x_m", model.bodyMaxX},
        {"roughness_m", model.roughness},
        {"perfect_finish", model.perfectFinish},
        {"body_lift_planform_area_m2", model.bodyLiftPlanformArea},
        {"body_lift_planform_moment_m3", model.bodyLiftPlanformMoment},
        {"symmetric_components", symmetric},
        {"fin_components", fins},
        {"tube_components", tubes},
        {"rail_button_components", buttons},
        {"tube_fin_components", tubeFins},
    };
}

inline Model fromJson(const json& value) {
    Model model;
    model.referenceArea = number(value, "reference_area_m2");
    model.aerodynamicLength = number(value, "aerodynamic_length_m");
    model.bodyWettedArea = number(value, "body_wetted_area_m2");
    model.otherWettedArea = number(value, "other_wetted_area_m2");
    model.pressureArea = number(value, "pressure_area_m2");
    model.baseArea = number(value, "base_area_m2");
    model.maxBodyRadius = number(value, "max_body_radius_m");
    model.bodyMinX = number(value, "body_min_x_m");
    model.bodyMaxX = number(value, "body_max_x_m");
    model.roughness = number(value, "roughness_m", 2.0e-6);
    model.perfectFinish = value.value("perfect_finish", false);
    model.bodyLiftPlanformArea = number(
        value, "body_lift_planform_area_m2");
    model.bodyLiftPlanformMoment = number(
        value, "body_lift_planform_moment_m3");
    for (const auto& c : value.value("symmetric_components", json::array())) {
        model.symmetricComponents.push_back({
            text(c, "id", ""), text(c, "parent_id"), number(c, "position_m"),
            text(c, "shape"), number(c, "length_m"),
            number(c, "fore_radius_m"), number(c, "aft_radius_m"),
            number(c, "shape_parameter"), number(c, "sin_phi"),
            number(c, "wetted_area_m2"), number(c, "planform_area_m2")});
    }
    for (const auto& c : value.value("fin_components", json::array())) {
        model.finComponents.push_back({
            text(c, "id", ""), static_cast<int>(number(c, "count")), number(c, "area_m2"),
            number(c, "span_m"), number(c, "thickness_m"),
            number(c, "mac_length_m"), number(c, "cos_leading_sweep", 1.0),
            text(c, "cross_section", "square"),
            number(c, "wetted_area_m2"), number(c, "planform_area_m2")});
    }
    for (const auto& c : value.value("tube_components", json::array())) {
        model.tubeComponents.push_back({
            text(c, "id", ""), number(c, "length_m"), number(c, "inner_radius_m"),
            number(c, "outer_radius_m"), number(c, "roughness_m", 2.0e-6),
            static_cast<int>(number(c, "count", 1.0)),
            number(c, "wetted_area_m2"), number(c, "planform_area_m2")});
    }
    for (const auto& c : value.value("rail_button_components", json::array())) {
        model.railButtonComponents.push_back({
            text(c, "id", ""), static_cast<int>(number(c, "count", 1.0)),
            number(c, "height_m", 0.0155), number(c, "outer_diameter_m", 0.012),
            number(c, "inner_diameter_m", 0.008), number(c, "inner_height_m", 0.005),
            number(c, "position_m", 0.0), number(c, "wetted_area_m2"), number(c, "planform_area_m2")});
    }
    for (const auto& c : value.value("tube_fin_components", json::array())) {
        model.tubeFinComponents.push_back({
            text(c, "id", ""), static_cast<int>(number(c, "count", 3.0)),
            number(c, "length_m", 0.1), number(c, "inner_radius_m", 0.025),
            number(c, "outer_radius_m", 0.027), number(c, "body_radius_m", 0.0625),
            number(c, "wetted_area_m2"), number(c, "interstice_area_m2"), number(c, "planform_area_m2")});
    }
    return model;
}

}  // namespace barrowman_drag
