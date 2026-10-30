from . import Model, discover_nodes
from .. import fins
from ..contracts import Source
LABELS=['Kök veter','Uç veter','Açıklık','Süpürme','Kalınlık','Kanat sayısı','Kesit','Yoğunluk','Gövde yarıçapı','Dil yüksekliği','Dil uzunluğu']
MODEL=Model('trapezoidal-fins','Trapez kanat seti',[Source(k,label,port) for (k,port),label in zip(fins.PORTS.items(),LABELS)],discover_nodes(fins),__file__,(fins.__file__,))
