import { BufferGeometry, Float32BufferAttribute, LatheGeometry, Vector2, Shape, ExtrudeGeometry } from 'three';
import { finProfile, noseProfile } from '../domain/geometry';
import type { Kind } from '../domain/project';
import {catalog,isStructural,type ComponentGeometry} from '../domain/componentCatalog';
export type GeometryFactory=(length:number,diameter:number,bodyDiameter:number,shape?:ComponentGeometry)=>BufferGeometry;
const revolved=(profile:[number,number][])=>{const g=new LatheGeometry(profile.map(([x,r])=>new Vector2(r*.001,x*.001)),48);g.rotateZ(-Math.PI/2);return g;};
const cylinder:GeometryFactory=(l,d,_,shape)=>{const inner=Math.min(d,shape?.innerDiameter??0)/2;return revolved([[0,inner],[0,d/2],[l,d/2],[l,inner],[0,inner]])};
const fin:GeometryFactory=(l,d,body,settings)=>{const shape=new Shape(finProfile(l,d/2,body/2,settings).map(([x,y])=>new Vector2(x*.001,y*.001)));const g=new ExtrudeGeometry(shape,{depth:.003,bevelEnabled:false});g.translate(0,0,-.0015);return g;};
export const geometryFactories:Partial<Record<Kind,GeometryFactory>>=Object.fromEntries(Object.keys(catalog).filter(kind=>!isStructural(kind as Kind)).map(kind=>[kind,cylinder]));
geometryFactories.nose=(l,d,_,shape)=>revolved([...noseProfile(l,d/2,shape?.noseProfile,shape?.noseParameter),[l,0]]);
geometryFactories.transition=(l,d,_,shape)=>revolved([[0,0],[0,d/2],[l,(shape?.aftDiameter??d*.8)/2],[l,0]]);
geometryFactories.fins=fin;geometryFactories.freeformFins=fin;
export function gridGeometry(){const vertices:number[]=[];for(let i=-50;i<=50;i++){const v=i*.1;vertices.push(-5,0,v,5,0,v,v,0,-5,v,0,5);}return new BufferGeometry().setAttribute('position',new Float32BufferAttribute(vertices,3));}
