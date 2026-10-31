from . import Model,discover_nodes
from .. import rail_button
from ..contracts import Source,DENSITY
LABELS=['Yükseklik','Dış yarıçap','İç yarıçap','Taban yüksekliği','Flanş yüksekliği','Vida yüksekliği','Tekrar sayısı','Tekrar aralığı']
MODEL=Model('rail-button','Ray düğmesi',[*[Source(k,l,p) for (k,p),l in zip(rail_button.PORTS.items(),LABELS)],Source('density','Yoğunluk',DENSITY)],discover_nodes(rail_button),__file__,(rail_button.__file__,))
