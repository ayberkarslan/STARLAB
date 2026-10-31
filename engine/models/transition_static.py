from . import Model,discover_nodes
from .. import static_transition
from ..contracts import Source
LABELS=['Uzunluk','Ön yarıçap','Arka yarıçap','Referans yarıçap','CNa override','CNa değeri','CP override','CP değeri']
MODEL=Model('transition-static-aero','Geçiş statik aerodinamiği',[Source(k,l,p) for (k,p),l in zip(static_transition.PORTS.items(),LABELS)],discover_nodes(static_transition),__file__,(static_transition.__file__,))
