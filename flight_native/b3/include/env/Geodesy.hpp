#pragma once

namespace geodesy {

struct GeodeticPosition {
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeM = 0.0;
};

struct EcefPosition {
    double xM = 0.0;
    double yM = 0.0;
    double zM = 0.0;
};

struct EnuPosition {
    double eastM = 0.0;
    double northM = 0.0;
    double upM = 0.0;
};

class Wgs84 {
public:
    static constexpr double semiMajorAxisM = 6378137.0;
    static constexpr double flattening = 1.0 / 298.257223563;
    static constexpr double semiMinorAxisM =
        semiMajorAxisM * (1.0 - flattening);
    static constexpr double firstEccentricitySquared =
        flattening * (2.0 - flattening);

    static EcefPosition geodeticToEcef(const GeodeticPosition& position);
    static GeodeticPosition ecefToGeodetic(const EcefPosition& position);

    // Rotate between inertial and Earth-fixed axes. The caller supplies the
    // Earth rotation angle so the conversion remains independent of epoch.
    static EcefPosition eciToEcef(
        const EcefPosition& eci, double earthRotationAngleRad);
    static EcefPosition ecefToEci(
        const EcefPosition& ecef, double earthRotationAngleRad);

    static EcefPosition enuToEcef(
        const EnuPosition& enu, const GeodeticPosition& origin);
    static EnuPosition ecefToEnu(
        const EcefPosition& ecef, const GeodeticPosition& origin);
    static GeodeticPosition enuToGeodetic(
        const EnuPosition& enu, const GeodeticPosition& origin);
    static EnuPosition geodeticToEnu(
        const GeodeticPosition& position, const GeodeticPosition& origin);

    // Azimuth is clockwise from true north; positive crossrange is right.
    static EnuPosition launchFrameToEnu(
        double downrangeM, double crossrangeM, double upM,
        double launchAzimuthDeg);
    static double horizontalDistanceM(const EnuPosition& enu) noexcept;
    static double bearingDeg(const EnuPosition& enu) noexcept;
    static double normalizeLongitudeDeg(double longitudeDeg) noexcept;
};

} // namespace geodesy
