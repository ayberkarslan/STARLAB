from . import Model,discover_nodes
from .. import transition
from ..contracts import Source,DENSITY
MODEL=Model('transition','Geçiş konisi',[*[Source(k,l,p) for (k,p),l in zip(transition.PORTS.items(),['Uzunluk','Ön yarıçap','Arka yarıçap','Et kalınlığı'])],Source('density','Yoğunluk',DENSITY)],discover_nodes(transition),__file__,(transition.__file__,))
