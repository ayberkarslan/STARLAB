import {resolveParts} from './geometry';
import type {Project,Part} from './project';
import type {GeometryEdit,GeometryPatch} from './geometryEdit';
export interface GeometrySnap {key:'offset'|'length'|'diameter';value:number;targetId:string;axis:'x'|'diameter';line:number}
export function snapCandidates(project:Project,part:Part,mode:GeometryEdit):GeometrySnap[]{
 const nodes=resolveParts(project),node=nodes.find(n=>n.part.id===part.id);if(!node)return [];
 const external=['body','nose'].includes(part.kind);
 const peers=nodes.filter(n=>n.part.id!==part.id&&n.physical&&(external?['body','nose'].includes(n.part.kind):n.part.kind===part.kind));
 if(mode==='diameter')return peers.map(n=>({key:'diameter',value:n.part.diameter,targetId:n.part.id,axis:'diameter',line:n.part.diameter/2}));
 // Descendants follow their parent; they cannot be a stationary alignment target.
 const descendants=new Set([part.id]);for(const n of nodes)if(n.parentId&&descendants.has(n.parentId))descendants.add(n.part.id);
 return peers.filter(n=>!descendants.has(n.part.id)).flatMap(n=>[n.position[0],n.position[0]+n.part.length].flatMap(line=>mode==='length'?[{key:'length' as const,value:line-node.position[0],targetId:n.part.id,axis:'x' as const,line}]:[0,part.length].map(edge=>({key:'offset' as const,value:part.offset+line-node.position[0]-edge,targetId:n.part.id,axis:'x' as const,line})))).filter(c=>c.key==='offset'||c.value>=1);
}
export function snapGeometry(patch:GeometryPatch,candidates:GeometrySnap[],worldPerPixel:number,previous?:GeometrySnap|null):{patch:GeometryPatch;snap:GeometrySnap|null}{
 const distance=(c:GeometrySnap)=>Math.abs((patch[c.key]??Infinity)-c.value)/(c.axis==='diameter'?2:1);
 const current=previous&&distance(previous)<=worldPerPixel*11?previous:null;
 const nearest=current??candidates.reduce<GeometrySnap|null>((best,c)=>distance(c)<=worldPerPixel*6&&(!best||distance(c)<distance(best))?c:best,null);
 return nearest?{patch:{...patch,[nearest.key]:nearest.value},snap:nearest}:{patch,snap:null};
}
