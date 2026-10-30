from dataclasses import replace
from ..contracts import Source
from ..aero import PRESSURE
from ..assembly import prepare as assembly
from ..coupling import attach
ID='rocket-aero'
def prepare(request,models,previous):
 base,_,values=assembly(request.get('assembly'),models)
 addon=models['aero-forces'];inputs=dict(request['inputs'])
 pressure=inputs.pop('pressure',None)
 if pressure is not None:
  addon=replace(addon,sources=[s for s in addon.sources if s.id!='density']+[Source('dynamicPressure','Girilen dinamik basınç',PRESSURE)],nodes=[n for n in addon.nodes if n.id!='dynamicPressure'])
  inputs.pop('density',None);inputs['dynamicPressure']=pressure
 model,graph,values=attach(base,addon,{'cg':'total_cg'},'aero_',values,inputs,previous)
 return replace(model,dependencies=(*model.dependencies,__file__)),graph,values
