import type {Project} from './project';
import type {GeometryEdit} from './geometryEdit';
import type {GeometrySnap} from './geometrySnap';
import {resolveParts} from './geometry';
import {geometrySelection,transformSelection} from './selectionGeometry';
export function selectionSnapCandidates(project:Project,ids:string[],mode:GeometryEdit):GeometrySnap[]{
 const selection=geometrySelection(project,ids),set=new Set(selection.nodes.map(n=>n.part.id));if(!set.size)return [];
 const diameter=Math.max(...selection.nodes.map(n=>n.part.diameter));
 const kinds=new Set(selection.nodes.filter(n=>mode!=='diameter'||n.part.diameter===diameter).map(n=>n.part.kind));
 const probe=new Map(resolveParts(transformSelection(project,ids,mode,1)).map(n=>[n.part.id,n.position[0]]));
 const peers=resolveParts(project).filter(n=>n.physical&&!set.has(n.part.id)&&kinds.has(n.part.kind)&&Math.abs((probe.get(n.part.id)??Infinity)-n.position[0])<.00001);
 if(mode==='diameter')return peers.map(n=>({key:'diameter',value:n.part.diameter,targetId:n.part.id,axis:'diameter',line:n.part.diameter/2}));
 const start=selection.bounds.min[0],width=selection.bounds.max[0]-start;const candidates:GeometrySnap[]=[];
 for(const peer of peers)for(const line of [peer.position[0],peer.position[0]+peer.part.length]){
  if(mode==='length'&&line>start)candidates.push({key:'length',value:line-start,targetId:peer.part.id,axis:'x',line});
  if(mode==='move')for(const edge of [0,width])candidates.push({key:'offset',value:line-start-edge,targetId:peer.part.id,axis:'x',line});
 }
 return candidates;
}
