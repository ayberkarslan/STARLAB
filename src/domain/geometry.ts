import {isStructural,isSerial} from './componentCatalog';
import { type Part, type Project } from './project';
export type Vec3 = [number,number,number];
export interface Bounds { min:Vec3; max:Vec3 }
export interface ResolvedPart { part:Part; parentId:string|null; localPosition:Vec3; position:Vec3; bounds:Bounds; physical:boolean }
export function resolveParts(project:Project):ResolvedPart[]{
 const byParent=new Map<string|null,Part[]>();for(const part of project.parts){const list=byParent.get(part.parentId)||[];list.push(part);byParent.set(part.parentId,list);}
 const resolved:ResolvedPart[]=[];
 function visit(part:Part,local:number,world:number):number {
  const physical=!isStructural(part.kind);
  const radius=Math.max(part.diameter,1)/2;
  const node:ResolvedPart={part,parentId:part.parentId,localPosition:[local,0,0],position:[world,0,0],physical,bounds:{min:[world,-radius,-radius],max:[world+Math.max(part.length,1),radius,radius]}};resolved.push(node);
  let cursor=0;let extent=physical?part.length:0;
  for(const child of byParent.get(part.id)||[]){const serial=isSerial(child.kind);const offset=(serial?cursor:0)+child.offset;const childEnd=visit(child,offset,world+offset);extent=Math.max(extent,offset+childEnd);if(serial)cursor=offset+childEnd;}
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
export function noseProfile(length:number,radius:number,profile?:import('./componentCatalog').NoseProfile,parameter?:number):[number,number][]{return Array.from({length:49},(_,i)=>{
 const t=i/48;if(!profile)return [length*(1.1*t-.1*t*t),radius*(2*t-t*t)];
 let r=t;
 if(profile==='elliptical')r=Math.sqrt(Math.max(0,2*t-t*t));
 if(profile==='parabolic'){const k=parameter??.5;r=(2*t-k*t*t)/Math.max(2-k,.001);}
 if(profile==='power')r=Math.pow(t,Math.max(parameter??.5,.01));
 if(profile==='tangentOgive'||profile==='secantOgive'){
  if(length>=radius&&radius>0){const a=profile==='tangentOgive'?1:Math.max(.01,parameter??.5),l=length/a;
   const rho=Math.sqrt(((length*length+radius*radius)*(((2-a)*length)**2+(a*radius)**2))/(4*(a*radius)**2));
   r=(Math.sqrt(Math.max(0,rho*rho-(l-length*t)**2))-Math.sqrt(Math.max(0,rho*rho-l*l)))/radius;
  }
 }
 if(profile==='haack'||profile==='vonKarman'){const theta=Math.acos(1-2*t),c=profile==='vonKarman'?0:parameter??0;r=Math.sqrt(Math.max(0,(theta-Math.sin(2*theta)/2+c*Math.sin(theta)**3)/Math.PI));}
 return [length*t,radius*Math.max(0,Math.min(1,r))];
});}
export function finProfile(length:number,radius:number,bodyRadius:number,shape?:import('./componentCatalog').ComponentGeometry):[number,number][]{
 if(shape?.points?.length){const xmax=Math.max(1,...shape.points.map(p=>p[0])),ymax=Math.max(1,...shape.points.map(p=>p[1]));return shape.points.map(([x,y])=>[x*length/xmax,bodyRadius+y*Math.max(0,radius-bodyRadius)/ymax]);}
 if(shape?.tipChord!==undefined||shape?.sweep!==undefined){const sweep=shape.sweep??length*.6,tip=shape.tipChord??length*.4;return [[0,bodyRadius],[sweep,Math.max(radius,bodyRadius)],[sweep+tip,Math.max(radius,bodyRadius)],[length,bodyRadius]];}
 return [[0,bodyRadius],[length*.6,Math.max(radius,bodyRadius)],[length,Math.max(radius,bodyRadius)],[length,bodyRadius]];
}
