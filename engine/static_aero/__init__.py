from ..contracts import Port

CNA=Port('normal-force-slope','1/rad',signed=True)
CP=Port('local-center-of-pressure','m','component-leading-origin',signed=True)
GLOBAL_CP=Port('global-center-of-pressure','m','vehicle-leading-origin',signed=True)
CP_MOMENT=Port('cna-cp-first-moment','m/rad','vehicle-leading-origin',signed=True)
MACH=Port('mach','1')
ANGLE=Port('angle-of-attack','rad',signed=True)
REFERENCE_RADIUS=Port('reference-radius','m',positive=True)
REFERENCE_AREA=Port('reference-area','m²',positive=True)
FLAG=Port('override-flag','1',integer=True,choices=(0,1))
