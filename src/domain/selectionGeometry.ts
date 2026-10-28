import {resolveParts,boundsOf} from './geometry';
import {subtree,validate,type Project,type Part} from './project';
import {selectionRoots} from './selection';
import {minimumDiameter,type GeometryEdit} from './geometryEdit';
const round=(n:number)=>Math.round(n*1000)/1000;
export function geometrySelection(project:Project,ids:string[]){
 const all=resolveParts(project),set=new Set(ids);
 for(const id of ids){const part=project.parts.find(p=>p.id===id);if(part&&['stage','assembly'].includes(part.kind))for(const p of subtree(project,id))set.add(p.id);}
 const nodes=all.filter(n=>set.has(n.part.id)&&n.physical);return {nodes,bounds:boundsOf(nodes)};
}
export function place(project:Project,targets:Map<string,number>){
 const byParent=new Map<string|null,Part[]>();for(const p of project.parts){const list=byParent.get(p.parentId)??[];list.push(p);byParent.set(p.parentId,list)}
 const changed=new Map<string,Part>();
 function visit(part:Part,world:number):number {
  let cursor=0,extent=['rocket','stage','assembly'].includes(part.kind)?0:part.length;
  for(const child of byParent.get(part.id)??[]){
   const serial=['stage','body','nose'].includes(child.kind);const base=serial?cursor:0;
   const wanted=targets.get(child.id);const offset=wanted===undefined?child.offset:round(wanted-world-base);
   if(Math.abs(offset-child.offset)>.00001)changed.set(child.id,{...child,offset});
   const local=base+offset,childEnd=visit(child,world+local);extent=Math.max(extent,local+childEnd);if(serial)cursor=local+childEnd;
  }
  return extent;
 }
 for(const root of byParent.get(null)??[])visit(root,0);
 return changed.size?{...project,parts:project.parts.map(p=>changed.get(p.id)??p)}:project;
}
/** Changes geometry inputs only. World positions are used to avoid double transforms. */
export function transformSelection(project:Project,ids:string[],mode:GeometryEdit,delta:number,step=1):Project {
 if(!Number.isFinite(delta)||!Number.isFinite(step)||step<=0||!ids.length)return project;
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
   factor=Math.max(factor,floor/Math.max(part[mode],1));
  }
  if(Math.abs(factor-1)<1e-12&&selection.nodes.every(n=>n.part[mode]>=1))return project;
  const patches=new Map(selection.nodes.map(n=>[n.part.id,{[mode]:Math.max(1,mode==='diameter'&&['body','nose','motor'].includes(n.part.kind)?2*(n.part.inputs?.wallThickness??0):1,round(n.part[mode]*factor))}]));
  if(mode==='diameter')for(const {part}of selection.nodes)if(part.kind==='fins'){
   const parentDiameter=patches.get(part.parentId??'')?.diameter??project.parts.find(p=>p.id===part.parentId)?.diameter??1;
   patches.set(part.id,{diameter:Math.max(patches.get(part.id)!.diameter!,parentDiameter)});
  }
  next={...project,parts:project.parts.map(p=>patches.has(p.id)?{...p,...patches.get(p.id)}:p)};
  if(mode==='length')next=place(next,new Map(selection.nodes.map(n=>[n.part.id,selection.bounds.min[0]+(n.position[0]-selection.bounds.min[0])*factor])));
 }
 if(next===project)return project;return validate(next);
}
