import math
from ..contracts import Port

class PlanformPort(Port):
    def parse(self,value):
        if not isinstance(value,list) or len(value)<6 or len(value)>1000 or len(value)%2:
            raise ValueError('Serbest kanat poligonu 3–500 nokta gerektirir.')
        if any(isinstance(n,bool) or not isinstance(n,(int,float)) or not math.isfinite(n) for n in value):
            raise ValueError('Serbest kanat koordinatları sonlu olmalı.')
        return [float(n) for n in value]
    def describe(self):return {**super().describe(),'kind':'xy-pairs'}

POINTS=PlanformPort('freeform-planform','m,m','component-leading-origin')
AREA=Port('planform-area','m²')
