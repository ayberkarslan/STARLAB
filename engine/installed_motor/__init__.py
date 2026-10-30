from ..contracts import Port
LOCAL_TIME=Port('motor-local-time','s',signed=True)
GLOBAL_TIME=Port('simulation-time','s')
DELAY=Port('ignition-delay','s')
