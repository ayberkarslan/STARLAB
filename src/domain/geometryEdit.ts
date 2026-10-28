import type {Part,Project} from './project';
export type GeometryEdit='move'|'length'|'diameter';
export type GeometryPatch=Partial<Pick<Part,'offset'|'length'|'diameter'>>;
export function minimumDiameter(project:Project,part:Part){return Math.max(1,['body','nose','motor'].includes(part.kind)?2*(part.inputs?.wallThickness??0):0,part.kind==='fins'?(project.parts.find(p=>p.id===part.parentId)?.diameter??0):0)}
export function geometryPatch(part:Part,mode:GeometryEdit,delta:number,step=1,diameterFloor=1):GeometryPatch {
 const key=mode==='move'?'offset':mode;
 const minimum=key==='offset'?-Infinity:key==='diameter'?Math.max(diameterFloor,['body','nose','motor'].includes(part.kind)?2*(part.inputs?.wallThickness??0):1):1;
 const value=Math.max(minimum,Math.round((part[key]+delta)/step)*step);
 return {[key]:Math.max(minimum,Math.round(value*1000)/1000)};
}
export function editGeometry(project:Project,id:string,patch:GeometryPatch):Project {
 const part=project.parts.find(p=>p.id===id);if(!part||['rocket','stage','assembly'].includes(part.kind))return project;
 for(const [key,value]of Object.entries(patch)){
  if(!['offset','length','diameter'].includes(key)||!Number.isFinite(value)||key!=='offset'&&value<1)throw Error('Geometri değeri geçersiz.');
 }
 if(patch.diameter!==undefined&&patch.diameter<minimumDiameter(project,part))throw Error('Çap et kalınlığını ve bağlı gövdeyi karşılamalı.');
 if(Object.entries(patch).every(([key,value])=>part[key as keyof GeometryPatch]===value))return project;
 return {...project,parts:project.parts.map(p=>p.id===id?{...p,...patch}:p)};
}
