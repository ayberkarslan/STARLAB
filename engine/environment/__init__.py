from ..contracts import Port
from ..aero import DENSITY,SPEED,MACH,PRESSURE as DYNAMIC_PRESSURE
class AltitudePort(Port):
 def parse(self,value):
  value=super().parse(value)
  if not -600<=value<=86000:raise ValueError('atmosphere altitude: −600…86000 m ASL destekleniyor')
  return value
ALTITUDE=AltitudePort('altitude-asl','m','mean-sea-level',signed=True)
HEIGHT=Port('height-agl','m','launch-site')
POSITIVE_HEIGHT=Port('positive-height','m',positive=True)
ATMOSPHERE_MODEL=Port('atmosphere-model','1',integer=True,choices=(0,1))
WIND_MODEL=Port('wind-model','1',integer=True,choices=(0,1,2))
TEMPERATURE=Port('temperature','K',positive=True)
PRESSURE=Port('atmosphere-pressure','Pa',positive=True)
SOUND=Port('speed-of-sound','m/s',positive=True)
VISCOSITY=Port('dynamic-viscosity','Pa·s',positive=True)
DIRECTION=Port('meteorological-direction','deg',signed=True)
GRADIENT=Port('wind-direction-gradient','deg/km',signed=True)
EXPONENT=Port('wind-shear-exponent','1')
MULTIPLIER=Port('wind-height-multiplier','1')
WIND_COMPONENT=Port('horizontal-wind-component','m/s','east-north-launch-plane',signed=True)
VELOCITY=Port('ground-or-air-velocity','m/s','launch-plane',signed=True)
REYNOLDS=Port('reynolds','1')
GRAVITY=Port('gravity','m/s²',positive=True)
def profile_multiplier(v,k):
 if v['minimum']>v['reference']:raise ValueError('Asgari yükseklik referansı aşamaz.')
 if v['exponent']>1:raise ValueError('Rüzgâr üssü 0–1 arasında olmalı.')
 if v['model']==2 and v['roughness']>=v['reference']:raise ValueError('Pürüzlülük referans yüksekliğinden küçük olmalı.')
 return k.environment_wind_multiplier(v['model'],v['height'],v['reference'],v['minimum'],v['exponent'],v['roughness'])
