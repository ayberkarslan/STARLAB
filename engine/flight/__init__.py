import math
from ..contracts import Port

class SeriesPort(Port):
    def parse(self,value):
        if not isinstance(value,list) or len(value)<8 or len(value)>20000 or len(value)%4:
            raise ValueError('Uçuş serisi en az iki zaman satırı gerektirir.')
        if any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) for v in value):
            raise ValueError('Uçuş serisi sonlu sayılardan oluşmalı.')
        if any(value[i]<=value[i-4] for i in range(4,len(value),4)):
            raise ValueError('Uçuş serisi zamanı kesin artmalı.')
        return [float(v) for v in value]
    def describe(self):return {**super().describe(),'kind':'number-array'}

SERIES=SeriesPort('flight-series','s,m,m/s,m/s²')
