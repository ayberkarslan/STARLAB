from ..contracts import LENGTH,WALL
from ..fins import COUNT,SIGNED_LENGTH
from ..static_aero import REFERENCE_AREA,MACH,ANGLE,FLAG,CNA,CP

PORTS={'root':LENGTH,'tip':WALL,'span':LENGTH,'sweep':SIGNED_LENGTH,'count':COUNT,'bodyRadius':WALL,
       'referenceArea':REFERENCE_AREA,'mach':MACH,'alpha':ANGLE,
       'cnaOverrideEnabled':FLAG,'cnaOverride':CNA,'cpOverrideEnabled':FLAG,'cpOverride':CP}
