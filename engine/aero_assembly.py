"""Compose component static-aero models and aggregate CNa-weighted CP."""
from dataclasses import replace
from hashlib import sha256
from .contracts import Source,Node,Binding,Port
from .models import Model
from .graph import Graph
from .static_aero import CNA,CP,GLOBAL_CP,CP_MOMENT

POSITION=Port('global-axial-position','m','vehicle-leading-origin',signed=True)
REFERENCE='RoketSiM src/cli/design_analysis.hpp::analyze · '

def prepare(parts,models):
 if not isinstance(parts,list) or not 1<=len(parts)<=64:raise ValueError('Aerodinamik hesap 1–64 bileşen destekliyor.')
 sources=[];nodes=[];values={};seen=set();cna_sum=None;moment_sum=None
 def node(id,label,output,bindings,fn,file,reference):
  nodes.append(Node(id,label,output,{k:Binding(ref,port) for k,(ref,port) in bindings.items()},fn,implementation='native/static_aero/'+file+'.cpp',reference=REFERENCE+reference))
 for part in parts:
  id=part.get('id');model_id=part.get('model')
  if not isinstance(id,str) or not id or id in seen or model_id not in models:raise ValueError('Geçersiz veya yinelenen aero bileşeni.')
  seen.add(id);model=models[model_id]
  if not {'cna','cp'}.issubset({n.id for n in model.nodes}):raise ValueError('Bu model aero katkısı olarak bağlanamaz.')
  prefix='a'+sha256(id.encode()).hexdigest()[:24]+'_';inputs=part.get('inputs')
  if not isinstance(inputs,dict) or set(inputs)-{s.id for s in model.sources}:raise ValueError('Aero bileşen girdileri geçersiz.')
  for source in model.sources:sources.append(replace(source,id=prefix+source.id));values[prefix+source.id]=inputs.get(source.id)
  for calculation in model.nodes:nodes.append(replace(calculation,id=prefix+calculation.id,inputs={k:replace(b,source=prefix+b.source) for k,b in calculation.inputs.items()},dependencies=(*calculation.dependencies,model.definition,*model.dependencies)))
  sources.append(Source(prefix+'position','Bileşen konumu',POSITION));values[prefix+'position']=part.get('position')
  node(prefix+'globalCp','Global CP',GLOBAL_CP,{'position':(prefix+'position',POSITION),'local':(prefix+'cp',CP)},lambda v,k:k.static_aero_global_cp(v['position'],v['local']),'global_cp','component position + local CP')
  node(prefix+'moment','CNa CP momenti',CP_MOMENT,{'cna':(prefix+'cna',CNA),'cp':(prefix+'globalCp',GLOBAL_CP)},lambda v,k:k.static_aero_moment(v['cna'],v['cp']),'moment','CNa × CP')
  current_cna,current_moment=prefix+'cna',prefix+'moment'
  if cna_sum:
   cna_id=prefix+'sum_cna';node(cna_id,'Toplam CNa',CNA,{'a':(cna_sum,CNA),'b':(current_cna,CNA)},lambda v,k:k.static_aero_sum(v['a'],v['b']),'sum','sum CNa');current_cna=cna_id
   moment_id=prefix+'sum_moment';node(moment_id,'Toplam CP momenti',CP_MOMENT,{'a':(moment_sum,CP_MOMENT),'b':(current_moment,CP_MOMENT)},lambda v,k:k.static_aero_sum(v['a'],v['b']),'sum','sum CNa × CP');current_moment=moment_id
  cna_sum,moment_sum=current_cna,current_moment
 node('total_cna','Roket CNa',CNA,{'a':(cna_sum,CNA)},lambda v,_:v['a'],'sum','final CNa')
 node('total_cp','Roket CP',GLOBAL_CP,{'cna':('total_cna',CNA),'moment':(moment_sum,CP_MOMENT)},lambda v,k:k.static_aero_centroid(v['cna'],v['moment']),'centroid','CP = Σ(CNa·CP)/ΣCNa')
 model=Model('static-aero-assembly','Statik aerodinamik',sources,nodes,__file__)
 return model,Graph(sources,nodes),values
