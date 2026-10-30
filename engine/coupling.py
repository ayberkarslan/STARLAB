"""Compose model DAGs by typed source bindings; independent of physical formulae."""
from dataclasses import replace
from .contracts import Binding
from .graph import Graph
from .models import Model

def attach(base,addon,bindings,prefix,values,inputs,previous=None):
 entries={e.id:e for e in [*base.sources,*base.nodes]}
 source_ids={s.id for s in addon.sources}
 if set(bindings)-source_ids or set(inputs)-(source_ids-set(bindings)):raise ValueError('Unknown coupled model input')
 for s in addon.sources:
  if s.id in bindings and (bindings[s.id] not in entries or s.output!=entries[bindings[s.id]].output):raise ValueError('Coupled port mismatch')
 sources=[*base.sources,*[replace(s,id=prefix+s.id) for s in addon.sources if s.id not in bindings]]
 nodes=[*base.nodes,*[replace(n,id=prefix+n.id,inputs={k:Binding(bindings.get(b.source,prefix+b.source),b.contract) for k,b in n.inputs.items()},dependencies=(*n.dependencies,addon.definition,*addon.dependencies)) for n in addon.nodes]]
 model=Model(base.id+'-'+addon.id,base.label+' · '+addon.label,sources,nodes,__file__,(base.definition,*base.dependencies))
 graph=Graph(sources,nodes)
 if previous:graph.cache,graph.versions,graph.revision=previous.cache,previous.versions,previous.revision
 return model,graph,{**values,**{prefix+s.id:inputs.get(s.id) for s in addon.sources if s.id not in bindings}}
