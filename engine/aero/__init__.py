from ..contracts import Port
from ..assembly import POSITION
DENSITY=Port('air-density','kg/m³',positive=True)
SPEED=Port('air-speed','m/s')
MACH=Port('mach','1')
class AnglePort(Port):
 def parse(self,value):
  value=super().parse(value)
  if abs(value)>3.141592653589793:raise ValueError('angle: −π…π gerekli')
  return value
ANGLE=AnglePort('angle','rad',signed=True)
RATE=Port('pitch-rate','rad/s',signed=True)
AREA=Port('body-planform-area','m²')
REF_AREA=Port('reference-area','m²',positive=True)
AREA_MOMENT=Port('body-planform-first-moment','m³',signed=True)
SLOPE=Port('normal-force-slope','1/rad',signed=True)
DRAG=Port('axial-drag-coefficient','1')
PRESSURE=Port('dynamic-pressure','Pa')
COEFFICIENT=Port('normal-coefficient','1',signed=True)
AXIAL_FORCE=Port('aero-axial-force','N','legacy-flight-body-forward',signed=True)
NORMAL_FORCE=Port('aero-normal-force','N','legacy-flight-body-normal',signed=True)
DISTANCE=Port('cp-cg-distance','m',signed=True)
MARGIN=Port('static-margin','cal',signed=True)
MOMENT=Port('pitch-moment','N·m','legacy-flight-pitch',signed=True)
