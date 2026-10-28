import {describe,it,expect} from 'vitest';
import {geometryFactories} from './geometry3d';
import {geometryResource,retainGeometry,releaseGeometry} from './resources3d';
import {resolveParts} from '../domain/geometry';
import {initialProject,type Part} from '../domain/project';
describe('3D geometry',()=>{
 it('uses the same length and radius in metres',()=>{for(const kind of ['body','motor','nose'] as const){const g=geometryFactories[kind]!(350,140,140);g.computeBoundingBox();expect(g.boundingBox!.min.x).toBeCloseTo(0,4);expect(g.boundingBox!.max.x).toBeCloseTo(.35,4);expect(g.boundingBox!.max.y).toBeCloseTo(.07,4);g.dispose()}});
 it('shares geometry and releases it only after the last user',()=>{const a=geometryResource('body',800,140,140);retainGeometry(a);const b=geometryResource('body',800,140,140);retainGeometry(b);expect(a).toBe(b);let disposed=0;a.geometry.addEventListener('dispose',()=>disposed++);releaseGeometry(a);expect(disposed).toBe(0);releaseGeometry(b);expect(disposed).toBe(1);const c=geometryResource('body',800,140,140);expect(c).not.toBe(a);retainGeometry(c);releaseGeometry(c)});
 it.each([100,500])('resolves and generates a %i-part scene without a browser',count=>{const p=initialProject();p.parts=p.parts.filter(n=>['rocket','sustainer'].includes(n.id));for(let i=0;i<count;i++)p.parts.push({id:`part-${i}`,parentId:'sustainer',kind:'body',name:'Body',length:100,diameter:140,offset:0,mass:0,material:'Tanımlanmadı'} as Part);const started=performance.now();const nodes=resolveParts(p);const resources=nodes.filter(n=>n.physical).map(n=>{const r=geometryResource(n.part.kind,n.part.length,n.part.diameter,140);retainGeometry(r);return r});expect(resources).toHaveLength(count);expect(new Set(resources).size).toBe(1);resources.forEach(releaseGeometry);console.info(`${count} parts: placement + shared CPU geometry ${(performance.now()-started).toFixed(2)} ms; GPU not measured`)});
});
