import {resolveParts,boundsOf} from './geometry';
import {subtree,validate,type Project} from './project';
import {selectionRoots} from './selection';
import {minimumDiameter,type GeometryEdit} from './geometryEdit';
const round=(n:number)=>Math.round(n*1000)/1000;
export function geometrySelection(project:Project,ids:string[]){
 const all=resolveParts(project),set=new Set(ids);
 for(const id of ids){const part=project.parts.find(p=>p.id===id);if(part&&['stage','assembly'].includes(part.kind))for(const p of subtree(project,id))set.add(p.id);}
 const nodes=all.filter(n=>set.has(n.part.id)&&n.physical);return {nodes,bounds:boundsOf(nodes)};
}
function place(project:Project,targets:Map<string,number>){
 let next=project;const order=resolveParts(project).filter(n=>targets.has(n.part.id)).map(n=>n.part.id);
 for(const id of order){const node=resolveParts(next).find(n=>n.part.id===id)!;const delta=targets.get(id)!-node.position[0];if(Math.abs(delta)<.00001)continue;next={...next,parts:next.parts.map(p=>p.id===id?{...p,offset:round(p.offset+delta)}:p)};}
 return next;
}
/** Changes geometry inputs only. World positions are used to avoid double transforms. */
export function transformSelection(project:Project,ids:string[],mode:GeometryEdit,delta:number,step=1):Project {
 if(!Number.isFinite(delta)||!ids.length)return project;
 const selection=geometrySelection(project,ids);if(!selection.nodes.length)return project;
 const all=resolveParts(project);let next=project;
 if(mode==='move'){
  const distance=Math.round(delta/step)*step;const roots=selectionRoots(project,ids).filter(p=>p.kind!=='rocket');
  next=place(project,new Map(roots.map(p=>[p.id,all.find(n=>n.part.id===p.id)!.position[0]+distance])));
 }else{
  const span=mode==='length'?selection.bounds.max[0]-selection.bounds.min[0]:Math.max(...selection.nodes.map(n=>n.part.diameter));
  const wanted=Math.round((span+delta)/step)*step;
  let factor=Math.max(.000001,wanted/Math.max(span,1));const selected=new Set(selection.nodes.map(n=>n.part.id));
  for(const {part}of selection.nodes){
   const floor=mode==='length'?1:part.kind==='fins'&&selected.has(part.parentId??'')?1:minimumDiameter(project,part);
   factor=Math.max(factor,floor/Math.max(part[mode],.001));
  }
  const patches=new Map(selection.nodes.map(n=>[n.part.id,{[mode]:round(n.part[mode]*factor)}]));
  next={...project,parts:project.parts.map(p=>patches.has(p.id)?{...p,...patches.get(p.id)}:p)};
  if(mode==='length')next=place(next,new Map(selection.nodes.map(n=>[n.part.id,selection.bounds.min[0]+(n.position[0]-selection.bounds.min[0])*factor])));
 }
 if(next===project)return project;return validate(next);
}
