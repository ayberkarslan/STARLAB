import {subtree,validate,removePart,type Part,type Project} from './project';
import {focusPath} from './navigation';
export function selectIds(current:string[],id:string,additive=false){return !id?[]:additive?(current.includes(id)?current.filter(v=>v!==id):[...current,id]):[id]}
export function selectionRoots(project:Project,ids:string[]){const set=new Set(ids);return project.parts.filter(p=>set.has(p.id)&&!focusPath(project,p.id).slice(0,-1).some(a=>set.has(a.id)))}
export function commonFocus(project:Project,ids:string[]){const paths=ids.map(id=>focusPath(project,id));return paths[0]?.filter((node,i)=>paths.every(path=>path[i]?.id===node.id)).at(-1)?.id??project.parts.find(p=>p.kind==='rocket')!.id}
export function scopedProject(project:Project,focus:string,query=''):Project {
 let parts=subtree(project,focus);if(!parts.length)parts=project.parts;
 if(query.trim()){
  const keep=new Set([focus]);for(const p of parts)if(p.name.toLocaleLowerCase('tr').includes(query.toLocaleLowerCase('tr').trim()))for(const node of focusPath(project,p.id))keep.add(node.id);
  parts=parts.filter(p=>keep.has(p.id));
 }
 const ids=new Set(parts.map(p=>p.id));return {...project,parts:parts.map(p=>p.parentId&&!ids.has(p.parentId)?{...p,parentId:null}:p)};
}
export function patchSelection(project:Project,ids:string[],patch:Partial<Part>):Project {
 const set=new Set(ids);const next={...project,parts:project.parts.map(p=>set.has(p.id)?{...p,...patch,geometry:patch.geometry?{...p.geometry,...patch.geometry}:p.geometry,inputs:patch.inputs?{...p.inputs,...patch.inputs}:p.inputs}:p)};
 return validate(next);
}
export function removeSelection(project:Project,ids:string[]){let next=project;for(const p of selectionRoots(project,ids))next=removePart(next,p.id);return next}
