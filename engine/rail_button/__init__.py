from ..contracts import LENGTH,WALL,Port
COUNT=Port('instance-count','count',integer=True,choices=tuple(range(1,65)))
PORTS={'height':LENGTH,'outerRadius':LENGTH,'innerRadius':WALL,'baseHeight':WALL,'flangeHeight':WALL,'screwHeight':WALL,'count':COUNT,'separation':WALL}
def validate(v):
 if v['innerRadius']>v['outerRadius']:raise ValueError('İç yarıçap dış yarıçapı aşamaz.')
