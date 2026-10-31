import type {Project} from '../domain/project';
import {isStructural} from '../domain/componentCatalog';
import {resolveParts} from '../domain/geometry';
import {hasStructuralModel,structuralInputs} from './structuralInputs';
import {allMotors,type FlightWorkspace} from '../domain/flight';
import {motorFits} from '../domain/motors';
import {motorInputs} from './motorInputs';
import type {AssemblyPart} from './protocol';
/** Coordinate/unit projection only. All mass/CG/inertia arithmetic runs native. */
export function rocketInputs(project:Project,flight?:FlightWorkspace,configurationId?:string,time=0,options:{allowMultipleStages?:boolean;ignoreSeparations?:boolean}={}){
 const issues:string[]=[],assembly:AssemblyPart[]=[];
 const config=configurationId?flight?.configurations.find(c=>c.id===configurationId):flight?.configurations[0];
 if(!Number.isFinite(time)||time<0)issues.push('Simülasyon zamanı geçersiz.');
 if(!options.ignoreSeparations&&config&&Object.values(config.separations).some(s=>s.event!=='never'))issues.push('Ayrılma bu yapı hesabına henüz bağlı değil.');
 if(!options.allowMultipleStages&&project.parts.filter(p=>p.kind==='stage').length!==1)issues.push('Bu yapı hesabı tek kademe destekliyor.');
 for(const p of project.parts){
  if(p.kind==='pod'||p.kind==='parallelStage'||(p.geometry?.radialOffset??0)!==0||((p.geometry?.instanceCount??1)!==1&&p.kind!=='railButton'))issues.push(`${p.name}: radyal veya tekrarlı yapı henüz bağlı değil.`);
  const supplied=p.inputs?.massProvided===true||p.inputs?.massProvided!==false&&p.mass>0;
  if(p.inputs?.massScope==='subtree'&&(supplied||p.inputs?.centerOfMass!==undefined))issues.push(`${p.name}: alt bileşenleri kapsayan kütle/CG override henüz bağlı değil.`);
 }
 const physical=resolveParts(project).filter(p=>!isStructural(p.part.kind));
 if(!physical.length)issues.push('Fiziksel bileşen ekle.');
 if(physical.length>64)issues.push('Bu yapı hesabı en fazla 64 fiziksel bileşen destekliyor.');
 for(const {part,position} of physical){
  const structuralPart=part.kind==='motor'?{...part,kind:'innerTube' as const}:part;
  if(!hasStructuralModel(structuralPart)){issues.push(`${part.name}: yapı hesabı henüz bağlı değil.`);continue}
  const request=structuralInputs(structuralPart,project);
  if(request.issue)issues.push(`${part.name}: ${request.issue}`);
  const missing=Object.entries(request.inputs).filter(([,v])=>v===undefined||!Number.isFinite(v)).map(([k])=>k);
  if(missing.length)issues.push(`${part.name}: eksik girdiler (${missing.join(', ')}).`);
  const supplied=part.inputs?.massProvided===true||part.inputs?.massProvided!==false&&part.mass>0;
  const overrides={...(supplied&&!['mass','parachute'].includes(part.kind)?{mass:part.mass/1000}:{}),...(part.inputs?.centerOfMass!==undefined?{cg:part.inputs.centerOfMass/1000}:{})};
  assembly.push({id:`part:${part.id}`,model:request.model,position:position[0]/1000,inputs:request.inputs,...(Object.keys(overrides).length?{overrides}:{})});
  if(part.kind!=='motor')continue;
  const assignment=config?.mounts[part.id],motor=flight&&allMotors(flight).find(m=>m.id===assignment?.motorId);
  if(!assignment||!motor){issues.push(`${part.name}: motor seçilmedi.`);continue}
  if(!motorFits(motor,part))issues.push(`${part.name}: motor ölçüleri yuvaya uymuyor.`);
  if(assignment.offset===undefined||!Number.isFinite(assignment.offset)||assignment.offset<0||assignment.offset+motor.length>part.length)issues.push(`${part.name}: motor başlangıcını yuva içinde belirt.`);
  if(assignment.event!=='launch'||!Number.isFinite(assignment.delay)||assignment.delay<0)issues.push(`${part.name}: yalnız fırlatmadan itibaren ateşleme gecikmesi destekleniyor.`);
  if(motor.massProfile?.length)issues.push(`${motor.name}: bağımsız kütle profili henüz bağlı değil.`);
  try{
   assembly.push({id:`motor:${part.id}`,model:'installed-motor',position:(position[0]+(assignment.offset??0))/1000,inputs:{...motorInputs(motor,time),delay:assignment.delay,length:motor.length/1000,radius:motor.diameter/2000}});
  }catch(error){issues.push(`${motor.name}: ${error instanceof Error?error.message:'Motor girdileri geçersiz.'}`)}
 }
 if(assembly.length>64)issues.push('Motorlar dahil en fazla 64 fiziksel katkı destekleniyor.');
 return {assembly,issues};
}
