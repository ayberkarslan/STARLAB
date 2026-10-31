from dataclasses import replace
from ..contracts import Source,Node,Binding,Port
from ..models import Model
from ..graph import Graph
from ..assembly import prepare as mass_assembly
from ..aero_assembly import prepare as aero_assembly,POSITION
from ..static_aero import GLOBAL_CP

ID='design'
LEVER=Port('stability-lever','m','vehicle-leading-origin',signed=True)
MARGIN=Port('stability-margin','cal',signed=True)
DIAMETER=Port('reference-diameter','m',positive=True)

def prepare(request,models,previous):
 base,_,values=mass_assembly(request.get('assembly'),models)
 aero,_,aero_values=aero_assembly(request.get('aeroAssembly'),models)
 if set(request['inputs'])!={'referenceDiameter'}:raise ValueError('Unknown design input')
 sources=[*base.sources,*aero.sources,Source('referenceDiameter','Referans çap',DIAMETER)]
 nodes=[*base.nodes,*aero.nodes]
 nodes.append(Node('stability_lever','CP−CG mesafesi',LEVER,{'cp':Binding('total_cp',GLOBAL_CP),'cg':Binding('total_cg',POSITION)},lambda v,k:k.static_aero_lever(v['cp'],v['cg']),implementation='native/static_aero/lever.cpp',reference='RoketSiM src/cli/roketsim_cli.cpp::deriv · CP−CG'))
 nodes.append(Node('stability_margin','Statik marj',MARGIN,{'lever':Binding('stability_lever',LEVER),'diameter':Binding('referenceDiameter',DIAMETER)},lambda v,k:k.static_aero_margin(v['lever'],v['diameter']),implementation='native/static_aero/margin.cpp',reference='RoketSiM src/cli/design_analysis.hpp::stability_margin_cal'))
 model=Model('design','Tasarım kütle ve kararlılık',sources,nodes,__file__,(base.definition,*base.dependencies,aero.definition,*aero.dependencies))
 graph=Graph(sources,nodes)
 if previous:graph.cache,graph.versions,graph.revision=previous.cache,previous.versions,previous.revision
 return model,graph,{**values,**aero_values,'referenceDiameter':request['inputs']['referenceDiameter']}
