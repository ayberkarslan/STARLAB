from . import Model,discover_nodes
from .. import static_fins
from ..contracts import Source

LABELS=['Kök veter','Uç veter','Açıklık','Süpürme','Kanat sayısı','Gövde yarıçapı','Referans alan','Mach','Hücum açısı','CNa override','CNa değeri','CP override','CP değeri']
MODEL=Model('fins-static-aero','Kanat statik aerodinamiği',[Source(k,l,p) for (k,p),l in zip(static_fins.PORTS.items(),LABELS)],discover_nodes(static_fins),__file__,(static_fins.__file__,))
