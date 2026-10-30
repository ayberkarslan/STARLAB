from ..contracts import Node,Binding
from . import ALTITUDE,ATMOSPHERE_MODEL,HEIGHT,TEMPERATURE
NODE=Node('temperature','Sıcaklık',TEMPERATURE,{'model':Binding('atmosphereModel',ATMOSPHERE_MODEL),'height':Binding('height',HEIGHT),'altitude':Binding('absoluteAltitude',ALTITUDE),'surfaceTemperature':Binding('surfaceTemperature',TEMPERATURE)},lambda v,k:k.environment_temperature(v['model'],v['height'],v['altitude'],v['surfaceTemperature']),implementation='native/environment/temperature.cpp',reference='RoketSiM src/env/Atmosphere.cpp')
