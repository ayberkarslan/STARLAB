from ..contracts import Node,Binding
from . import LOCAL_TIME,GLOBAL_TIME,DELAY
NODE=Node('localTime','Motor yerel zamanı',LOCAL_TIME,{'time':Binding('time',GLOBAL_TIME),'delay':Binding('delay',DELAY)},lambda v,k:k.motor_local_time(v['time'],v['delay']),implementation='native/motor/local_time.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp: t - resolved_ignition_time_s')
