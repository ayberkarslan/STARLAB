from . import Model, discover_nodes
from .. import mass_component
from ..contracts import Source, Port, LENGTH

MODEL = Model('mass-component', 'Kütle bileşeni', [
    Source('length', 'Uzunluk', LENGTH), Source('radius', 'Yarıçap', LENGTH),
    Source('suppliedMass', 'Girilen kütle', Port('positive-mass', 'kg', positive=True)),
], discover_nodes(mass_component), __file__)
