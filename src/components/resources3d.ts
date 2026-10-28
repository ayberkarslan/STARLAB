import {EdgesGeometry,MeshBasicMaterial,MeshStandardMaterial,LineBasicMaterial,type BufferGeometry} from 'three';
import {geometryFactories} from './geometry3d';
import type {Kind} from '../domain/project';
interface Resource { key:string; geometry:BufferGeometry; edges:EdgesGeometry; users:number }
const cache=new Map<string,Resource>();
export function geometryResource(kind:Kind,length:number,diameter:number,bodyDiameter:number):Resource {
 const key=[kind,length,diameter,kind==='fins'?bodyDiameter:0].join(':');
 const existing=cache.get(key);if(existing)return existing;
 const factory=geometryFactories[kind];if(!factory)throw Error('Unsupported geometry');
 const geometry=factory(Math.max(length,1),Math.max(diameter,1),bodyDiameter);
 const resource={key,geometry,edges:new EdgesGeometry(geometry,20),users:0};cache.set(key,resource);return resource;
}
export function retainGeometry(resource:Resource){resource.users++;if(!cache.has(resource.key))cache.set(resource.key,resource);}
export function releaseGeometry(resource:Resource){resource.users--;if(resource.users===0){resource.geometry.dispose();resource.edges.dispose();if(cache.get(resource.key)===resource)cache.delete(resource.key);}}
export function sceneMaterials(){return {surface:new MeshStandardMaterial({color:'#242424',roughness:.38,metalness:.12,polygonOffset:true,polygonOffsetFactor:1,polygonOffsetUnits:1}),edge:new LineBasicMaterial({color:'white'}),selected:new MeshBasicMaterial({color:'white',wireframe:true})};}
export type SceneMaterials=ReturnType<typeof sceneMaterials>;
