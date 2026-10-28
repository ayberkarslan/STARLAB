import type {Part,Project} from './project';
export type GeometryEdit='move'|'length'|'diameter';
export type GeometryPatch=Partial<Pick<Part,'offset'|'length'|'diameter'>>;
export function geometryPatch(part:Part,mode:GeometryEdit,delta:number,step=1):GeometryPatch {
 const key=mode==='move'?'offset':mode;
 const minimum=key==='offset'?-Infinity:key==='diameter'&&['body','nose','motor'].includes(part.kind)?Math.max(1,2*(part.inputs?.wallThickness??0)):1;
 const value=Math.max(minimum,Math.round((part[key]+delta)/step)*step);
 return {[key]:Math.round(value*1000)/1000};
}
export function editGeometry(project:Project,id:string,patch:GeometryPatch):Project {
 const part=project.parts.find(p=>p.id===id);if(!part||['rocket','stage','assembly'].includes(part.kind))return project;
 for(const [key,value]of Object.entries(patch)){
  if(!['offset','length','diameter'].includes(key)||!Number.isFinite(value)||key!=='offset'&&value<1)throw Error('Geometri değeri geçersiz.');
 }
 if(patch.diameter!==undefined&&['body','nose','motor'].includes(part.kind)&&patch.diameter<2*(part.inputs?.wallThickness??0))throw Error('Çap et kalınlığını karşılamalı.');
 if(Object.entries(patch).every(([key,value])=>part[key as keyof GeometryPatch]===value))return project;
 return {...project,parts:project.parts.map(p=>p.id===id?{...p,...patch}:p)};
}
