import {catalog,validate,type Project} from './project';
import {selectionRoots} from './selection';
import {resolveParts} from './geometry';
import {place} from './selectionGeometry';
export function canGroup(project:Project,ids:string[]){
 const roots=selectionRoots(project,ids),parent=project.parts.find(p=>p.id===roots[0]?.parentId);
 return roots.length>0&&!!parent&&['stage','assembly'].includes(parent.kind)&&roots.every(p=>p.parentId===parent.id&&catalog[p.kind].parents.includes('assembly'));
}
export function groupSelection(project:Project,ids:string[],id=crypto.randomUUID()){
 if(!canGroup(project,ids))throw Error('Aynı kademe veya alt sistem içindeki uyumlu bileşenleri seç.');
 const roots=selectionRoots(project,ids),set=new Set(roots.map(p=>p.id)),nodes=resolveParts(project),parent=nodes.find(n=>n.part.id===roots[0].parentId)!;
 const start=Math.min(...nodes.filter(n=>set.has(n.part.id)).map(n=>n.position[0]));
 const group={id,parentId:parent.part.id,kind:'assembly' as const,name:'Alt sistem',length:0,diameter:0,offset:start-parent.position[0],mass:0,material:'Tanımlanmadı'};
 const parts=project.parts.flatMap(p=>p.id===roots[0].id?[group,{...p,parentId:id}]:[{...p,parentId:set.has(p.id)?id:p.parentId}]);
 const next=place({...project,parts},new Map(nodes.map(n=>[n.part.id,n.position[0]])));
 return {project:validate(next),id};
}
