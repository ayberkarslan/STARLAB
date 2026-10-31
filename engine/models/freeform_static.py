from . import Model,discover_nodes
from .. import freeform_static
from ..contracts import Source

LABELS=['Kanat poligonu','Kanat sayısı','Gövde yarıçapı','Referans alan','Mach','Hücum açısı','CNa override','CNa değeri','CP override','CP değeri']
MODEL=Model('freeform-static-aero','Serbest kanat statik aerodinamiği',
 [Source(k,l,p) for (k,p),l in zip(freeform_static.PORTS.items(),LABELS)],discover_nodes(freeform_static),__file__,(freeform_static.__file__,))
