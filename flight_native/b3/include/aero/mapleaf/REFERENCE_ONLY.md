# MAPLEAF reference-only material

These headers are preserved as MIT-attributed parity/reference material from
MAPLEAF commit `af970d3e8200832f5e70d537b15ad38dd74fa551`.  They are not part
of `RocketEngine` and must not be selected as an aerodynamic provider.

RoketSiM's canonical static model owns Barrowman CNa, CP, interference, drag,
and transonic handling.  The production-build dynamic extension is
`aero/DynamicFinAero.hpp`; it returns a delta over a caller-owned canonical
component coefficient and uses no MAPLEAF static/interference/drag formula.
