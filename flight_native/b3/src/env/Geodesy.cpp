#include "env/Geodesy.hpp"

#include <cmath>
#include <stdexcept>

namespace geodesy {
namespace {
constexpr double pi = 3.14159265358979323846;
double radians(double value) { return value * pi / 180.0; }
double degrees(double value) { return value * 180.0 / pi; }

void validate(const GeodeticPosition& p) {
    if (!std::isfinite(p.latitudeDeg) || p.latitudeDeg < -90.0
        || p.latitudeDeg > 90.0) {
        throw std::invalid_argument("WGS84 latitude must be between -90 and 90");
    }
    if (!std::isfinite(p.longitudeDeg) || !std::isfinite(p.altitudeM)) {
        throw std::invalid_argument("WGS84 longitude and altitude must be finite");
    }
}
}

double Wgs84::normalizeLongitudeDeg(double longitudeDeg) noexcept {
    double result = std::fmod(longitudeDeg + 180.0, 360.0);
    if (result < 0.0) result += 360.0;
    return result - 180.0;
}

EcefPosition Wgs84::geodeticToEcef(const GeodeticPosition& p) {
    validate(p);
    const double lat = radians(p.latitudeDeg);
    const double lon = radians(normalizeLongitudeDeg(p.longitudeDeg));
    const double sinLat = std::sin(lat);
    const double cosLat = std::cos(lat);
    const double n = semiMajorAxisM / std::sqrt(
        1.0 - firstEccentricitySquared * sinLat * sinLat);
    return {
        (n + p.altitudeM) * cosLat * std::cos(lon),
        (n + p.altitudeM) * cosLat * std::sin(lon),
        (n * (1.0 - firstEccentricitySquared) + p.altitudeM) * sinLat,
    };
}

GeodeticPosition Wgs84::ecefToGeodetic(const EcefPosition& p) {
    if (!std::isfinite(p.xM) || !std::isfinite(p.yM)
        || !std::isfinite(p.zM)) {
        throw std::invalid_argument("ECEF coordinates must be finite");
    }
    const double horizontal = std::hypot(p.xM, p.yM);
    if (horizontal < 1.0e-9) {
        return {
            p.zM >= 0.0 ? 90.0 : -90.0,
            0.0,
            std::abs(p.zM) - semiMinorAxisM,
        };
    }
    const double ep2 = (
        semiMajorAxisM * semiMajorAxisM
        - semiMinorAxisM * semiMinorAxisM)
        / (semiMinorAxisM * semiMinorAxisM);
    const double theta = std::atan2(
        p.zM * semiMajorAxisM, horizontal * semiMinorAxisM);
    const double sinTheta = std::sin(theta);
    const double cosTheta = std::cos(theta);
    const double lat = std::atan2(
        p.zM + ep2 * semiMinorAxisM * sinTheta * sinTheta * sinTheta,
        horizontal - firstEccentricitySquared * semiMajorAxisM
            * cosTheta * cosTheta * cosTheta);
    const double lon = std::atan2(p.yM, p.xM);
    const double sinLat = std::sin(lat);
    const double n = semiMajorAxisM / std::sqrt(
        1.0 - firstEccentricitySquared * sinLat * sinLat);
    const double altitude = horizontal / std::cos(lat) - n;
    return {
        degrees(lat),
        normalizeLongitudeDeg(degrees(lon)),
        altitude,
    };
}

EcefPosition Wgs84::eciToEcef(
    const EcefPosition& eci, double earthRotationAngleRad) {
    if (!std::isfinite(eci.xM) || !std::isfinite(eci.yM)
        || !std::isfinite(eci.zM)
        || !std::isfinite(earthRotationAngleRad)) {
        throw std::invalid_argument(
            "ECI coordinates and Earth rotation angle must be finite");
    }
    const double c = std::cos(earthRotationAngleRad);
    const double s = std::sin(earthRotationAngleRad);
    return {
        c * eci.xM + s * eci.yM,
        -s * eci.xM + c * eci.yM,
        eci.zM,
    };
}

EcefPosition Wgs84::ecefToEci(
    const EcefPosition& ecef, double earthRotationAngleRad) {
    if (!std::isfinite(ecef.xM) || !std::isfinite(ecef.yM)
        || !std::isfinite(ecef.zM)
        || !std::isfinite(earthRotationAngleRad)) {
        throw std::invalid_argument(
            "ECEF coordinates and Earth rotation angle must be finite");
    }
    const double c = std::cos(earthRotationAngleRad);
    const double s = std::sin(earthRotationAngleRad);
    return {
        c * ecef.xM - s * ecef.yM,
        s * ecef.xM + c * ecef.yM,
        ecef.zM,
    };
}

EcefPosition Wgs84::enuToEcef(
    const EnuPosition& enu, const GeodeticPosition& origin) {
    validate(origin);
    const auto reference = geodeticToEcef(origin);
    const double lat = radians(origin.latitudeDeg);
    const double lon = radians(normalizeLongitudeDeg(origin.longitudeDeg));
    const double sinLat = std::sin(lat), cosLat = std::cos(lat);
    const double sinLon = std::sin(lon), cosLon = std::cos(lon);
    return {
        reference.xM - sinLon * enu.eastM
            - sinLat * cosLon * enu.northM + cosLat * cosLon * enu.upM,
        reference.yM + cosLon * enu.eastM
            - sinLat * sinLon * enu.northM + cosLat * sinLon * enu.upM,
        reference.zM + cosLat * enu.northM + sinLat * enu.upM,
    };
}

EnuPosition Wgs84::ecefToEnu(
    const EcefPosition& ecef, const GeodeticPosition& origin) {
    validate(origin);
    const auto reference = geodeticToEcef(origin);
    const double dx = ecef.xM - reference.xM;
    const double dy = ecef.yM - reference.yM;
    const double dz = ecef.zM - reference.zM;
    const double lat = radians(origin.latitudeDeg);
    const double lon = radians(normalizeLongitudeDeg(origin.longitudeDeg));
    const double sinLat = std::sin(lat), cosLat = std::cos(lat);
    const double sinLon = std::sin(lon), cosLon = std::cos(lon);
    return {
        -sinLon * dx + cosLon * dy,
        -sinLat * cosLon * dx - sinLat * sinLon * dy + cosLat * dz,
        cosLat * cosLon * dx + cosLat * sinLon * dy + sinLat * dz,
    };
}

GeodeticPosition Wgs84::enuToGeodetic(
    const EnuPosition& enu, const GeodeticPosition& origin) {
    return ecefToGeodetic(enuToEcef(enu, origin));
}

EnuPosition Wgs84::geodeticToEnu(
    const GeodeticPosition& p, const GeodeticPosition& origin) {
    return ecefToEnu(geodeticToEcef(p), origin);
}

EnuPosition Wgs84::launchFrameToEnu(
    double downrangeM, double crossrangeM, double upM,
    double launchAzimuthDeg) {
    if (!std::isfinite(downrangeM) || !std::isfinite(crossrangeM)
        || !std::isfinite(upM) || !std::isfinite(launchAzimuthDeg)) {
        throw std::invalid_argument("launch-frame coordinates must be finite");
    }
    const double azimuth = radians(launchAzimuthDeg);
    return {
        downrangeM * std::sin(azimuth) + crossrangeM * std::cos(azimuth),
        downrangeM * std::cos(azimuth) - crossrangeM * std::sin(azimuth),
        upM,
    };
}

double Wgs84::horizontalDistanceM(const EnuPosition& enu) noexcept {
    return std::hypot(enu.eastM, enu.northM);
}

double Wgs84::bearingDeg(const EnuPosition& enu) noexcept {
    if (std::abs(enu.eastM) < 1.0e-12
        && std::abs(enu.northM) < 1.0e-12) return 0.0;
    double result = degrees(std::atan2(enu.eastM, enu.northM));
    if (result < 0.0) result += 360.0;
    return result;
}

} // namespace geodesy
