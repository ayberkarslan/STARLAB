from . import Model,discover_nodes
from .. import freeform
from ..contracts import Source

MODEL=Model('freeform-planform','Serbest kanat planformu',
 [Source('points','Kanat poligonu',freeform.POINTS)],discover_nodes(freeform),__file__,(freeform.__file__,))
