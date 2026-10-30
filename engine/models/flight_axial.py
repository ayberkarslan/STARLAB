from . import Model
from ..contracts import Source,MASS
from ..motor import CURVE
from ..flight.trajectory import NODE,NONNEG,POSITIVE,SIGNED,ATMOSPHERE

MODEL=Model('flight-axial','Tek kademe 2B RK4 uçuş',[
 Source('curve','İtki eğrisi',CURVE),Source('wetMass','Kalkış kütlesi',MASS),Source('propellant','Yakıt kütlesi',MASS),
 Source('cd','Eksenel Cd',NONNEG),Source('area','Referans alanı',POSITIVE),Source('atmosphereModel','Atmosfer modeli',ATMOSPHERE),
 Source('launchAltitude','Saha irtifası',SIGNED),Source('surfaceTemperature','Yüzey sıcaklığı',POSITIVE),Source('surfacePressure','Yüzey basıncı',POSITIVE),
 Source('windSpeed','Rüzgâr hızı',NONNEG),Source('windDirection','Rüzgâr yönü',NONNEG),Source('heading','Ray yönü',NONNEG),Source('railAngle','Ray açısı',NONNEG),Source('railLength','Ray uzunluğu',POSITIVE),
 Source('dt','Zaman adımı',POSITIVE),Source('maxTime','Azami süre',POSITIVE),
], [NODE], __file__, (__import__('engine.flight.trajectory',fromlist=['']).__file__,))
