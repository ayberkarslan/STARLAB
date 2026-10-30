from ..contracts import Node,Binding
from ..motor import CURVE,FORCE
from . import LOCAL_TIME
NODE=Node('thrust','Atanan motor itkisi',FORCE,{'curve':Binding('curve',CURVE),'time':Binding('localTime',LOCAL_TIME)},lambda v,k:k.motor_installed_thrust(v['curve'],len(v['curve'])//2,v['time']),implementation='native/motor/installed_thrust.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp: ignition gate + lerp')
