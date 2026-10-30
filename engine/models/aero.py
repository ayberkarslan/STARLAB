from . import Model,discover_nodes
from .. import aero
from ..contracts import Source,LENGTH
MODEL=Model('aero-forces','Girilen katsayılarla aero',[
 Source('density','Hava yoğunluğu',aero.DENSITY),Source('speed','Akış hızı',aero.SPEED),Source('mach','Mach',aero.MACH),
 Source('alpha','Hücum açısı',aero.ANGLE),Source('area','Referans alanı',aero.REF_AREA),Source('diameter','Referans çapı',LENGTH),
 Source('cna','Roket CNa',aero.SLOPE),Source('cd','Roket eksenel Cd',aero.DRAG),Source('cp','Roket CP',aero.POSITION),Source('cg','Roket CG',aero.POSITION),
 Source('bodyArea','Gövde planform alanı',aero.AREA),Source('bodyMoment','Gövde planform ilk momenti',aero.AREA_MOMENT),Source('pitchRate','Yunuslama hızı',aero.RATE)
],discover_nodes(aero),__file__,(aero.__file__,))
