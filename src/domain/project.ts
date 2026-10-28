import {validateFlight,cleanFlight,type FlightWorkspace} from './flight';
export type Kind = 'rocket'|'stage'|'assembly'|'body'|'nose'|'fins'|'motor'|'parachute'|'cord';
export interface Part { id:string; parentId:string|null; kind:Kind; name:string; length:number; diameter:number; offset:number; mass:number; material:string }
export interface Project { version:1; name:string; parts:Part[]; flight?:FlightWorkspace }
export const catalog:Record<Kind,{label:string; parents:Kind[]; length:number; diameter:number}> = {
 rocket:{label:'Roket',parents:[],length:0,diameter:0}, stage:{label:'Kademe',parents:['rocket'],length:0,diameter:0},
 assembly:{label:'Alt sistem',parents:['stage','assembly'],length:0,diameter:0}, body:{label:'Gövde tüpü',parents:['stage','assembly'],length:800,diameter:140},
 nose:{label:'Burun konisi',parents:['stage','assembly'],length:350,diameter:140}, fins:{label:'Kanat seti',parents:['body'],length:220,diameter:280},
 motor:{label:'Motor yuvası',parents:['body','assembly'],length:450,diameter:98},parachute:{label:'Paraşüt',parents:['body','assembly'],length:100,diameter:100},cord:{label:'Kurtarma ipi',parents:['body','assembly'],length:100,diameter:20}
};
export function children(p:Project,id:string){return p.parts.filter(n=>n.parentId===id)}
export function subtree(p:Project,id:string):Part[]{const n=p.parts.find(n=>n.id===id);return n?[n,...children(p,id).flatMap(c=>subtree(p,c.id))]:[]}
export function addPart(p:Project,parentId:string,kind:Kind):Project { const parent=p.parts.find(n=>n.id===parentId);if(!parent||!catalog[kind].parents.includes(parent.kind))throw Error('Bu parçaya eklenemez.');return {...p,parts:[...p.parts,{id:crypto.randomUUID(),parentId,kind,name:catalog[kind].label,length:catalog[kind].length,diameter:catalog[kind].diameter,offset:0,mass:0,material:'Tanımlanmadı'}]}; }
export function removePart(p:Project,id:string):Project {if(p.parts.find(n=>n.id===id)?.kind==='rocket')throw Error('Roket silinemez.');const ids=new Set(subtree(p,id).map(n=>n.id));const next={...p,parts:p.parts.filter(n=>!ids.has(n.id))};if(next.flight)next.flight=cleanFlight(next,next.flight);return next;}
export function validate(value:unknown):Project {
 const p=value as Project;if(!p||p.version!==1||typeof p.name!=='string'||!Array.isArray(p.parts)||!p.parts.length||p.parts.length>2000)throw Error('Geçersiz proje dosyası.');
 const ids=new Set<string>();for(const n of p.parts){if(!n||typeof n.id!=='string'||ids.has(n.id)||!Object.hasOwn(catalog,n.kind)||typeof n.name!=='string'||typeof n.material!=='string'||![n.length,n.diameter,n.offset,n.mass].every(Number.isFinite)||n.length<0||n.diameter<0||n.mass<0)throw Error('Parça verileri geçersiz.');ids.add(n.id);}
 if(p.parts.filter(n=>n.kind==='rocket').length!==1)throw Error('Tek roket kökü gerekli.');
 for(const n of p.parts){if(n.kind==='rocket'){if(n.parentId!==null)throw Error('Geçersiz kök.');continue;}const parent=p.parts.find(a=>a.id===n.parentId);if(!parent||!catalog[n.kind].parents.includes(parent.kind))throw Error('Geçersiz parça bağlantısı.');const seen=new Set([n.id]);let current:Part|undefined=parent;while(current){if(seen.has(current.id))throw Error('Döngüsel parça bağlantısı.');seen.add(current.id);current=p.parts.find(a=>a.id===current?.parentId);}}
 if(p.flight!==undefined){validateFlight(p.flight);const cleaned=cleanFlight(p,p.flight);if(JSON.stringify(cleaned.configurations)!==JSON.stringify(p.flight.configurations))throw Error('Uçuş ayarı yanlış bir bileşene bağlı.');}
 return p;
}
export function initialProject():Project {
 const parts:Part[]=[];const put=(id:string,parentId:string|null,kind:Kind,name:string,length=0,diameter=140,offset=0)=>parts.push({id,parentId,kind,name,length,diameter,offset,mass:0,material:'Tanımlanmadı'});
 put('rocket',null,'rocket','Roket');put('sustainer','rocket','stage','Üst kademe · Sustainer');put('nose','sustainer','nose','Burun konisi',350);put('upper','sustainer','body','Üst kademe gövdesi',1000);put('avionics','sustainer','assembly','Aviyonik & kurtarma');put('chute','avionics','parachute','Ana paraşüt',120,90,180);put('cord','avionics','cord','Kurtarma ipi',100,20,320);put('ufins','upper','fins','Üst kademe kanatları',220,280,740);put('umotor','upper','motor','Üst kademe motor yuvası',450,98,530);put('booster','rocket','stage','Alt kademe · Booster');put('lower','booster','body','Booster gövdesi',850);put('lfins','lower','fins','Booster kanatları',260,320,560);put('lmotor','lower','motor','Booster motor yuvası',600,98,230);put('recovery','booster','assembly','Booster kurtarma');put('bchute','recovery','parachute','Booster paraşütü',100,90,120);
 return {version:1,name:'İlk tasarım',parts};
}
