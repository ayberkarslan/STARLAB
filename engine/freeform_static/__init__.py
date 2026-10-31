from ..freeform import POINTS
from ..fins import COUNT
from ..contracts import WALL
from ..static_aero import REFERENCE_AREA,MACH,ANGLE,FLAG,CNA,CP

PORTS={'points':POINTS,'count':COUNT,'bodyRadius':WALL,'referenceArea':REFERENCE_AREA,'mach':MACH,'alpha':ANGLE,
       'cnaOverrideEnabled':FLAG,'cnaOverride':CNA,'cpOverrideEnabled':FLAG,'cpOverride':CP}
