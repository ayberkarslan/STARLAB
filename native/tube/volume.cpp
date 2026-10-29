// Extracted from RoketSiM src/components/BodyTube.cpp::volume (SI).
extern "C" double tube_volume(double radius, double thickness, double length) {
    const double inner = radius - thickness;
    constexpr double pi = 3.141592653589793238462643383279502884;
    return pi * (radius * radius - inner * inner) * length;
}
