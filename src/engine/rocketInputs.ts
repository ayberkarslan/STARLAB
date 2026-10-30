import type {Project} from '../domain/project';
import {isStructural} from '../domain/componentCatalog';
import {resolveParts} from '../domain/geometry';
import {hasStructuralModel,structuralInputs} from './structuralInputs';
import type {AssemblyPart} from './protocol';
/** Coordinate/unit projection only. All mass/CG/inertia arithmetic runs native. */
export function rocketInputs(project:Project){
 const issues:string[]=[],assembly:AssemblyPart[]=[];
 if(project.parts.filter(p=>p.kind==='stage').length!==1)issues.push('Bu yapı hesabı tek kademe destekliyor.');
 for(const p of project.parts){
  if(p.kind==='pod'||p.kind==='parallelStage'||(p.geometry?.radialOffset??0)!==0||(p.geometry?.instanceCount??1)!==1)issues.push(`${p.name}: radyal veya tekrarlı yapı henüz bağlı değil.`);
  const supplied=p.inputs?.massProvided===true||p.inputs?.massProvided!==false&&p.mass>0;
  if((p.kind!=='mass'&&supplied)||p.inputs?.centerOfMass!==undefined)issues.push(`${p.name}: kütle/CG düzeltmesi henüz bağlı değil.`);
 }
 const physical=resolveParts(project).filter(p=>!isStructural(p.part.kind));
 if(!physical.length)issues.push('Fiziksel bileşen ekle.');
 if(physical.length>64)issues.push('Bu yapı hesabı en fazla 64 fiziksel bileşen destekliyor.');
 for(const {part,position} of physical){
  if(!hasStructuralModel(part)){issues.push(`${part.name}: yapı hesabı henüz bağlı değil.`);continue}
  const request=structuralInputs(part,project);
  if(request.issue)issues.push(`${part.name}: ${request.issue}`);
  const missing=Object.entries(request.inputs).filter(([,v])=>v===undefined||!Number.isFinite(v)).map(([k])=>k);
  if(missing.length)issues.push(`${part.name}: eksik girdiler (${missing.join(', ')}).`);
  assembly.push({id:part.id,model:request.model,position:position[0]/1000,inputs:request.inputs});
 }
 return {assembly,issues};
}
