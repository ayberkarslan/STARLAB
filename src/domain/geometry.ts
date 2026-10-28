import { type Part, type Project } from './project';
export type Vec3 = [number,number,number];
export interface Bounds { min:Vec3; max:Vec3 }
export interface ResolvedPart { part:Part; parentId:string|null; localPosition:Vec3; position:Vec3; bounds:Bounds; physical:boolean }
export function resolveParts(project:Project):ResolvedPart[]{
 const byParent=new Map<string|null,Part[]>();for(const part of project.parts){const list=byParent.get(part.parentId)||[];list.push(part);byParent.set(part.parentId,list);}
 const resolved:ResolvedPart[]=[];
 function visit(part:Part,local:number,world:number):number {
  const physical=!['rocket','stage','assembly'].includes(part.kind);
  const radius=Math.max(part.diameter,1)/2;
  const node:ResolvedPart={part,parentId:part.parentId,localPosition:[local,0,0],position:[world,0,0],physical,bounds:{min:[world,-radius,-radius],max:[world+Math.max(part.length,1),radius,radius]}};resolved.push(node);
  let cursor=0;let extent=physical?part.length:0;
  for(const child of byParent.get(part.id)||[]){const serial=['stage','body','nose'].includes(child.kind);const offset=(serial?cursor:0)+child.offset;const childEnd=visit(child,offset,world+offset);extent=Math.max(extent,offset+childEnd);if(serial)cursor=offset+childEnd;}
  return extent;
 }
 for(const root of byParent.get(null)||[])visit(root,0,0);
 return resolved;
}
export function scopeParts(parts:ResolvedPart[],focus:string){const ids=new Set([focus]);for(const n of parts)if(n.parentId&&ids.has(n.parentId))ids.add(n.part.id);return parts.filter(n=>ids.has(n.part.id));}
export function boundsOf(parts:ResolvedPart[]):Bounds {
 const physical=parts.filter(n=>n.physical);if(!physical.length)return {min:[0,-50,-50],max:[100,50,50]};
 return {min:[0,1,2].map(i=>Math.min(...physical.map(n=>n.bounds.min[i]))) as Vec3,max:[0,1,2].map(i=>Math.max(...physical.map(n=>n.bounds.max[i]))) as Vec3};
}
// Identical profile samples feed the SVG outline and the revolved 3D nose.
export function noseProfile(length:number,radius:number):[number,number][]{return Array.from({length:33},(_,i)=>{const t=i/32;return [length*(1.1*t-.1*t*t),radius*(2*t-t*t)];});}
export function finProfile(length:number,radius:number,bodyRadius:number):[number,number][]{return [[0,bodyRadius],[length*.6,Math.max(radius,bodyRadius)],[length,Math.max(radius,bodyRadius)],[length,bodyRadius]];}
