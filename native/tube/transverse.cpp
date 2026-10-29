// RoketSiM BodyTube::rawMassProperties Ixx = Iyy, about local CG.
extern "C" double tube_transverse(double mass, double radius, double thickness, double length) {
    const double inner = radius - thickness;
    return (1.0 / 12.0) * mass * (3.0 * (radius * radius + inner * inner) + length * length);
}
