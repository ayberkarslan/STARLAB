from ..contracts import Port, LENGTH, WALL, DENSITY
COUNT=Port('fin-count','count',integer=True,choices=tuple(range(1,65)))
SECTION=Port('fin-section','1',integer=True,choices=(0,1,2,3,4))
SIGNED_LENGTH=Port('signed-length','m',signed=True)
AREA=Port('planform-area','m²')
FACTOR=Port('section-volume-factor','1',positive=True)
SIGNED_CG=Port('local-axial-cg-signed','m','component-leading-origin',signed=True)
PORTS={'root':LENGTH,'tip':WALL,'span':LENGTH,'sweep':SIGNED_LENGTH,'thickness':LENGTH,'count':COUNT,'sectionCode':SECTION,'density':DENSITY,'bodyRadius':WALL,'tabHeight':WALL,'tabLength':WALL}
