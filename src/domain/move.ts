import {catalog,subtree,type Project} from './project';

export type Placement='inside'|'before'|'after';
export interface MoveTarget { id:string; placement:Placement }
export function moveError(project:Project,sourceId:string,target:MoveTarget):string|null {
 const source=project.parts.find(p=>p.id===sourceId),destination=project.parts.find(p=>p.id===target.id);
 if(!source||!destination)return 'Bileşen bulunamadı.';
 if(source.kind==='rocket')return 'Roket kökü taşınamaz.';
 if(subtree(project,sourceId).some(p=>p.id===target.id))return 'Bir bileşen kendisinin veya alt bileşeninin içine taşınamaz.';
 const parent=target.placement==='inside'?destination:project.parts.find(p=>p.id===destination.parentId);
 if(!parent||!catalog[source.kind].parents.includes(parent.kind))return `${catalog[source.kind].label} bu konuma yerleştirilemez.`;
 return null;
}
export function movePart(project:Project,sourceId:string,target:MoveTarget):Project {
 const error=moveError(project,sourceId,target);if(error)throw Error(error);
 const destination=project.parts.find(p=>p.id===target.id)!;
 const branch=subtree(project,sourceId);const ids=new Set(branch.map(p=>p.id));const remaining=project.parts.filter(p=>!ids.has(p.id));
 const parentId=target.placement==='inside'?destination.id:destination.parentId;
 let index=remaining.findIndex(p=>p.id===destination.id);
 if(target.placement!=='before'){
  const targetIds=new Set(subtree({...project,parts:remaining},destination.id).map(p=>p.id));
  index=remaining.reduce((last,p,i)=>targetIds.has(p.id)?i+1:last,index+1);
 }
 const next=[...remaining.slice(0,index),...branch.map(p=>p.id===sourceId?{...p,parentId}:p),...remaining.slice(index)];
 if(next.every((p,i)=>p.id===project.parts[i].id&&p.parentId===project.parts[i].parentId))return project;
 return {...project,parts:next};
}
