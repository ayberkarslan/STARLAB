import {} from './componentCatalog';
import {resolveParts} from './geometry';
import type {Project,Part} from './project';
import type {GeometryEdit,GeometryPatch} from './geometryEdit';
export interface GeometrySnap {key:'offset'|'length'|'diameter';value:number;targetId:string;axis:'x'|'diameter';line:number}
export function snapCandidates(project:Project,part:Part,mode:GeometryEdit):GeometrySnap[]{
 const nodes=resolveParts(project),node=nodes.find(n=>n.part.id===part.id);if(!node)return [];
 const external=['body','nose','transition'].includes(part.kind);
 const peers=nodes.filter(n=>n.part.id!==part.id&&n.physical&&(external?['body','nose','transition'].includes(n.part.kind):n.part.kind===part.kind));
 if(mode==='diameter')return peers.sort((a,b)=>Number(b.part.kind===part.kind)-Number(a.part.kind===part.kind)).map(n=>({key:'diameter',value:n.part.diameter,targetId:n.part.id,axis:'diameter',line:n.part.diameter/2}));
 // Descendants follow their parent; they cannot be a stationary alignment target.
 const descendants=new Set([part.id]);for(const n of nodes)if(n.parentId&&descendants.has(n.parentId))descendants.add(n.part.id);
 const changed=mode==='move'?'offset':'length';
 const probe=new Map(resolveParts({...project,parts:project.parts.map(p=>p.id===part.id?{...p,[changed]:p[changed]+1}:p)}).map(n=>[n.part.id,n.position[0]]));
 const result:GeometrySnap[]=[];
 for(const peer of peers){
  if(descendants.has(peer.part.id)||Math.abs((probe.get(peer.part.id)??Infinity)-peer.position[0])>1e-6)continue;
  for(const line of [peer.position[0],peer.position[0]+peer.part.length]){
   if(mode==='length')result.push({key:'length',value:line-node.position[0],targetId:peer.part.id,axis:'x',line});
   else for(const edge of [0,part.length])result.push({key:'offset',value:part.offset+line-node.position[0]-edge,targetId:peer.part.id,axis:'x',line});
  }
 }
 return result.filter(c=>c.key==='offset'||c.value>=1);
}
export function snapGeometry(patch:GeometryPatch,candidates:GeometrySnap[],worldPerPixel:number,previous?:GeometrySnap|null):{patch:GeometryPatch;snap:GeometrySnap|null}{
 const distance=(c:GeometrySnap)=>Math.abs((patch[c.key]??Infinity)-c.value)/(c.axis==='diameter'?2:1);
 const current=previous&&distance(previous)<=worldPerPixel*11?previous:null;
 const nearest=current??candidates.reduce<GeometrySnap|null>((best,c)=>distance(c)<=worldPerPixel*6&&(!best||distance(c)<distance(best))?c:best,null);
 return nearest?{patch:{...patch,[nearest.key]:nearest.value},snap:nearest}:{patch,snap:null};
}
