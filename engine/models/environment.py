from . import Model,discover_nodes
from .. import environment as e
from ..contracts import Source,LENGTH
MODEL=Model('environment','Atmosfer ve rüzgâr',[
 Source('height','Fırlatma noktasından yükseklik',e.HEIGHT),Source('launchAltitude','Fırlatma sahası irtifası',e.ALTITUDE),Source('atmosphereModel','Atmosfer modeli',e.ATMOSPHERE_MODEL),
 Source('surfaceTemperature','Yüzey sıcaklığı',e.TEMPERATURE),Source('surfacePressure','Yüzey basıncı',e.PRESSURE),
 Source('windModel','Rüzgâr modeli',e.WIND_MODEL),Source('meanSpeed','Referans rüzgâr hızı',e.SPEED),Source('direction','Rüzgârın geldiği yön',e.DIRECTION),Source('directionGradient','Yön değişimi',e.GRADIENT),Source('heading','Ray azimutu',e.DIRECTION),
 Source('referenceHeight','Rüzgâr referans yüksekliği',e.POSITIVE_HEIGHT),Source('minimumHeight','Asgari rüzgâr yüksekliği',e.HEIGHT),Source('exponent','Rüzgâr üssü',e.EXPONENT),Source('roughness','Yüzey pürüzlülüğü',e.POSITIVE_HEIGHT),
 Source('groundDownrange','Yere göre yatay hız',e.VELOCITY),Source('groundVertical','Yere göre dikey hız',e.VELOCITY),Source('referenceLength','Reynolds referans uzunluğu',LENGTH)
],discover_nodes(e),__file__,(e.__file__,))
