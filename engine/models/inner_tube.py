from . import Model, discover_nodes
from .. import inner_tube
from ..contracts import Source, LENGTH, DENSITY

MODEL = Model('inner-tube', 'İç tüp', [
    Source('length', 'Uzunluk', LENGTH), Source('radius', 'Dış yarıçap', LENGTH),
    Source('innerRadius', 'İç yarıçap', LENGTH), Source('density', 'Yoğunluk', DENSITY),
], discover_nodes(inner_tube), __file__)
