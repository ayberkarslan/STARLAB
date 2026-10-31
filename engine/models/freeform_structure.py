from . import Model,discover_nodes
from .. import freeform_structure,freeform
from ..fins import section,volume,mass,axial,transverse
from ..contracts import Source

LABELS=['Kanat poligonu','Kanat sayısı','Kalınlık','Kesit','Yoğunluk','Gövde yarıçapı','Dil yüksekliği','Dil uzunluğu']
MODEL=Model('freeform-fins','Serbest kanat seti',
 [Source(k,l,p) for (k,p),l in zip(freeform_structure.PORTS.items(),LABELS)],
 [freeform.area.NODE,*discover_nodes(freeform_structure),section.NODE,volume.NODE,mass.NODE,axial.NODE,transverse.NODE],
 __file__,(freeform_structure.__file__,freeform.__file__))
