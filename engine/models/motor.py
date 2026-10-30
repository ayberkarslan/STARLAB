from . import Model,discover_nodes
from .. import motor
from ..contracts import Source,MASS,Port
MODEL=Model('motor','Motor',[Source('curve','İtki eğrisi',motor.CURVE),Source('time','Motor yerel zamanı',motor.TIME),Source('wet','Başlangıç toplam kütlesi',Port('positive-motor-mass','kg',positive=True)),Source('propellant','Başlangıç yakıt kütlesi',MASS)],discover_nodes(motor),__file__,(motor.__file__,))
