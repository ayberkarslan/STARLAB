// RoketSiM BodyTube::rawMassProperties Izz. Adapter labels axial axis +X.
extern "C" double tube_axial(double mass, double radius, double thickness) {
    const double inner = radius - thickness;
    return 0.5 * mass * (radius * radius + inner * inner);
}
