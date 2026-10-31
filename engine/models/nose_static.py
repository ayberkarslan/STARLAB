from . import Model,discover_nodes
from .. import static_nose
from ..contracts import Source

LABELS=['Uzunluk','Taban yarıçapı','Profil','Referans yarıçap','CNa override','CNa değeri','CP override','CP değeri']
MODEL=Model('nose-static-aero','Burun statik aerodinamiği',[Source(k,l,p) for (k,p),l in zip(static_nose.PORTS.items(),LABELS)],discover_nodes(static_nose),__file__,(static_nose.__file__,))
