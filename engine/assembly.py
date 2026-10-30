"""Build a scalar DAG from existing part models; no UI or physics equations here."""
from dataclasses import replace
from hashlib import sha256
from .contracts import Source, Node, Binding, Port, MASS, INERTIA
from .models import Model
from .graph import Graph
POSITION=Port('global-axial-position','m','vehicle-leading-origin',signed=True)
ORIGIN_INERTIA=Port('origin-inertia','kg·m²','vehicle-leading-origin')
TOTAL_INERTIA=Port('vehicle-centroidal-inertia','kg·m²','vehicle-cg')
MOMENT=Port('mass-first-moment','kg·m','vehicle-leading-origin',signed=True)
REFERENCE='RoketSiM src/cli/runtime_mass_properties.hpp: OriginAccumulator::'

def prepare(parts,models,previous=None):
    if not isinstance(parts,list) or not 1<=len(parts)<=64:
        raise ValueError('Yapı hesabı 1–64 fiziksel bileşen destekliyor.')
    sources=[];nodes=[];values={};seen=set();accum={}
    def add_node(id,label,output,bindings,fn,file,reference):
        nodes.append(Node(id,label,output,{k:Binding(ref,port) for k,(ref,port) in bindings.items()},fn,implementation='native/assembly/'+file+'.cpp',reference=REFERENCE+reference))
    for part in parts:
        id=part.get('id');model_id=part.get('model')
        if not isinstance(id,str) or not id or len(id)>512 or id in seen or model_id not in models:
            raise ValueError('Geçersiz veya yinelenen yapı bileşeni.')
        seen.add(id);m=models[model_id];prefix='p'+sha256(id.encode()).hexdigest()[:24]+'_'
        inputs=part.get('inputs')
        if not isinstance(inputs,dict) or set(inputs)-{s.id for s in m.sources}:raise ValueError('Bileşen girdileri geçersiz.')
        for s in m.sources:
            sources.append(replace(s,id=prefix+s.id));values[prefix+s.id]=inputs.get(s.id)
        for n in m.nodes:
            nodes.append(replace(n,id=prefix+n.id,inputs={k:replace(b,source=prefix+b.source) for k,b in n.inputs.items()},dependencies=(*n.dependencies,m.definition,*m.dependencies)))
        sources.append(Source(prefix+'position','Bileşen konumu',POSITION));values[prefix+'position']=part.get('position')
        cg_port=next(n.output for n in m.nodes if n.id=='cg')
        add_node(prefix+'globalCg','Global CG',POSITION,{'a':(prefix+'position',POSITION),'b':(prefix+'cg',cg_port)},lambda v,k:k.assembly_sum(v['a'],v['b']),'sum','add (position + local CG)')
        add_node(prefix+'moment','İlk moment',MOMENT,{'mass':(prefix+'mass',MASS),'x':(prefix+'globalCg',POSITION)},lambda v,k:k.assembly_moment(v['mass'],v['x']),'moment','add · mx')
        add_node(prefix+'origin','Başlangıç etrafında atalet',ORIGIN_INERTIA,{'mass':(prefix+'mass',MASS),'x':(prefix+'globalCg',POSITION),'inertia':(prefix+'transverse',INERTIA)},lambda v,k:k.assembly_origin(v['mass'],v['x'],v['inertia']),'origin','add · iyy')
        add_node(prefix+'axialOrigin','Eksenel atalet',ORIGIN_INERTIA,{'value':(prefix+'axial',INERTIA)},lambda v,_:v['value'],'sum','add · ixx (coaxial)')
        for name,output,ref in [('mass',MASS,'mass'),('moment',MOMENT,'moment'),('axial',ORIGIN_INERTIA,'axialOrigin'),('origin',ORIGIN_INERTIA,'origin')]:
            current=prefix+ref
            if name in accum:
                target=prefix+'sum_'+name
                add_node(target,'Toplam '+name,output,{'a':(accum[name],output),'b':(current,output)},lambda v,k:k.assembly_sum(v['a'],v['b']),'sum','merge')
                current=target
            accum[name]=current
    for name,output,source_port in [('mass',MASS,MASS),('axial',TOTAL_INERTIA,ORIGIN_INERTIA)]:
        add_node('total_'+name,'Toplam '+name,output,{'value':(accum[name],source_port)},lambda v,_:v['value'],'sum','merge')
    bindings={'mass':(accum['mass'],MASS),'moment':(accum['moment'],MOMENT)}
    add_node('total_cg','Roket CG',POSITION,bindings,lambda v,k:k.assembly_cg(v['mass'],v['moment']),'centroid','finalize · cg_x_m')
    add_node('total_transverse','Toplam enine atalet',TOTAL_INERTIA,{**bindings,'origin':(accum['origin'],ORIGIN_INERTIA)},lambda v,k:k.assembly_transverse(v['mass'],v['moment'],v['origin']),'transverse','finalize · iyy_kgm2')
    model=Model('rocket','Toplam yapı',sources,nodes,__file__);graph=Graph(sources,nodes)
    if previous:
        graph.cache,graph.versions,graph.revision=previous.cache,previous.versions,previous.revision
    return model,graph,values
