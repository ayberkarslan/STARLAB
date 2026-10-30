from dataclasses import replace
from ..assembly import prepare as assembly
from ..coupling import attach
ID='flight'

def prepare(request,models,previous):
    base,_,values=assembly(request.get('assembly'),models)
    model,graph,values=attach(base,models['flight-axial'],{'wetMass':'total_mass'},'flight_',values,dict(request['inputs']),previous)
    return replace(model,dependencies=(*model.dependencies,__file__)),graph,values
