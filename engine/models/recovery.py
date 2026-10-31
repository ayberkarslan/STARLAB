from . import Model,discover_nodes
from .. import recovery
from ..contracts import Source
MODEL=Model('packed-recovery','Paketlenmiş kurtarma elemanı',[Source(k,l,p) for (k,p),l in zip(recovery.PORTS.items(),['Paket boyu','Paket kütlesi'])],discover_nodes(recovery),__file__,(recovery.__file__,))
