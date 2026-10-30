from ..contracts import Port, LENGTH, WALL, DENSITY
PROFILE=Port('nose-profile', '1', integer=True, choices=(0,1,2,3,4,5))
FLAG=Port('flag', '1', integer=True, choices=(0,1))
PARAMETER=Port('profile-parameter', '1')
PORTS={'length':LENGTH,'radius':LENGTH,'wall':WALL,'profile':PROFILE,'parameter':PARAMETER,'filled':FLAG,'shoulderLength':WALL,'shoulderRadius':WALL,'shoulderWall':WALL,'capped':FLAG}

def validate(v):
    if v['filled']==0 and v['wall']>v['radius']:
        raise ValueError('Et kalınlığı dış yarıçapı aşamaz.')
    if v['shoulderWall']>v['shoulderRadius']:
        raise ValueError('Omuz et kalınlığı omuz yarıçapını aşamaz.')
    if v['shoulderLength']>0 and v['shoulderRadius']<=0:
        raise ValueError('Omuz çapı gerekli.')
    if v['profile']==4 and not 0<v['parameter']<=1:
        raise ValueError('Üs profili parametresi 0–1 aralığında ve pozitif olmalı.')
    if v['profile']==3 and v['parameter']>1:
        raise ValueError('Parabolik profil parametresi 0–1 aralığında olmalı.')
    if v['profile']==5 and v['parameter']>1/3:
        raise ValueError('Haack parametresi 0–1/3 aralığında olmalı.')
