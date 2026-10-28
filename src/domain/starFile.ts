import {validate,type Project} from './project';
import {allMotors,initialFlight,simulationSignature} from './flight';

export const STAR_MAX_BYTES=50*1024*1024;
export interface StarFile {format:'starlab';schemaVersion:1;project:Project}
/** Bundle referenced built-in motors too: the file must not depend on a future catalog. */
export function portableProject(project:Project):Project {
 const flight=project.flight??initialFlight();
 const referenced=new Set(flight.configurations.flatMap(c=>Object.values(c.mounts).map(a=>a.motorId)));
 const bundled=allMotors(flight).filter(m=>referenced.has(m.id)&&!flight.motors.some(own=>own.id===m.id));
 const used=new Set(flight.motors.map(m=>m.id));const ids=new Map<string,string>();
 for(const m of bundled){let id=`bundled-${m.id}`;while(used.has(id))id+='-copy';used.add(id);ids.set(m.id,id);}
 const next={...project,flight:{...flight,motors:[...flight.motors,...bundled.map(m=>({...m,id:ids.get(m.id)!}))],configurations:flight.configurations.map(c=>({...c,mounts:Object.fromEntries(Object.entries(c.mounts).map(([id,a])=>[id,{...a,motorId:ids.get(a.motorId)??a.motorId}]))}))}};
 next.flight.cases=flight.cases.map(c=>{
  const config=flight.configurations.find(cfg=>cfg.id===c.configurationId)!;
  if(!c.result||c.result.signature!==simulationSignature(project,config,c.environment,flight))return c;
  const updated=next.flight.configurations.find(cfg=>cfg.id===c.configurationId)!;
  return {...c,result:{...c.result,signature:simulationSignature(next,updated,c.environment,next.flight)}};
 });
 return next;
}
export function serializeStar(project:Project):string {
 const data:StarFile={format:'starlab',schemaVersion:1,project:validate(portableProject(project))};
 const text=JSON.stringify(data,null,2);
 if(new TextEncoder().encode(text).byteLength>STAR_MAX_BYTES)throw Error('Proje dosyası 50 MB sınırını aşıyor.');
 return text;
}
export function parseStar(text:string):Project {
 if(new TextEncoder().encode(text).byteLength>STAR_MAX_BYTES)throw Error('Proje dosyası 50 MB sınırını aşıyor.');
 let value;try{value=JSON.parse(text.replace(/^\uFEFF/,''));}catch{throw Error('Proje dosyası okunamadı.');}
 if(value&&Object.hasOwn(value,'format')){
  if(value.format!=='starlab'||value.schemaVersion!==1)throw Error('Bu proje biçimi veya sürümü desteklenmiyor.');
  return validate(value.project);
 }
 // Earlier STARLAB JSON exports remain readable.
 return validate(value);
}
export function starFilename(name:string){return `${name.replace(/[<>:"/\\|?*\u0000-\u001f]/g,'-').trim().slice(0,100)||'proje'}.star`;}
