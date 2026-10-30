from . import Model,discover_nodes
from .. import annular
from ..contracts import Source
MODEL=Model('annular-cylinder','Halka / bölme',[Source(k,label,port) for (k,port),label in zip(annular.PORTS.items(),['Eksenel kalınlık','Dış yarıçap','İç yarıçap','Yoğunluk'])],discover_nodes(annular),__file__,(annular.__file__,))
