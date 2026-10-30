import math
from ..contracts import Port

class CurvePort(Port):
    def parse(self,value):
        if not isinstance(value,list) or len(value)<4 or len(value)>20000 or len(value)%2:
            raise ValueError('İtki eğrisi 2–10000 zaman/itki çifti gerektirir.')
        if any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) or v<0 for v in value):
            raise ValueError('İtki eğrisi değerleri sonlu ve negatif olmayan sayılar olmalı.')
        if any(value[i]<=value[i-2] for i in range(2,len(value),2)):
            raise ValueError('İtki eğrisinde zaman kesin artmalı.')
        return [float(v) for v in value]
    def describe(self):return {**super().describe(),'kind':'number-array'}
CURVE=CurvePort('thrust-curve','s,N')
TIME=Port('motor-local-time','s')
FORCE=Port('thrust-force','N')
IMPULSE=Port('thrust-impulse','N·s')
