// RoketSiM BodyTube::rawMassProperties, mass = material density * volume.
extern "C" double tube_mass(double volume, double density) { return density * volume; }
