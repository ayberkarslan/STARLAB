from . import Model, discover_nodes
from .. import nose
from ..contracts import Source, DENSITY
LABELS=['Uzunluk','Taban yarıçapı','Et kalınlığı','Profil','Profil parametresi','Dolu yapı','Omuz uzunluğu','Omuz yarıçapı','Omuz et kalınlığı','Omuz kapağı']
MODEL=Model('nose','Burun konisi',[*[Source(k,label,port) for (k,port),label in zip(nose.PORTS.items(),LABELS)],Source('density','Yoğunluk',DENSITY)],discover_nodes(nose),__file__,(nose.__file__,))
