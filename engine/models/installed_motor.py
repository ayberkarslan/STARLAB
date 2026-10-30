from dataclasses import replace
from . import Model,discover_nodes
from .. import installed_motor,motor
from ..contracts import Source,Binding,LENGTH
from .motor import MODEL as MOTOR
# Reuse existing nodes; only the time binding changes from local input to resolved local time.
base=[]
for n in MOTOR.nodes:
 if n.id=='thrust':continue
 inputs={key:Binding('localTime',installed_motor.LOCAL_TIME) if b.source=='time' else b for key,b in n.inputs.items()}
 base.append(replace(n,inputs=inputs,dependencies=(*n.dependencies,MOTOR.definition,*MOTOR.dependencies)))
sources=[replace(s,label='Simülasyon zamanı',output=installed_motor.GLOBAL_TIME) if s.id=='time' else s for s in MOTOR.sources]
sources += [Source('delay','Ateşleme gecikmesi',installed_motor.DELAY),Source('length','Motor uzunluğu',LENGTH),Source('radius','Motor yarıçapı',LENGTH)]
MODEL=Model('installed-motor','Atanan motor',sources,[*base,*discover_nodes(installed_motor)],__file__,(installed_motor.__file__,motor.__file__))
