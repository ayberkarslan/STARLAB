from ..contracts import LENGTH,WALL
PORTS={'length':LENGTH,'foreRadius':LENGTH,'aftRadius':LENGTH,'wall':WALL}

def validate(v):
 if v['wall']>=min(v['foreRadius'],v['aftRadius']):raise ValueError('Et kalınlığı iki uç yarıçapından da küçük olmalı.')
