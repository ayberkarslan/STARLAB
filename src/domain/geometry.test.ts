import {describe,it,expect} from 'vitest';
import {initialProject} from './project';
import {resolveParts,scopeParts,boundsOf,noseProfile} from './geometry';
describe('shared placement',()=>{
 it('places both stages and keeps child offsets relative to parents',()=>{const p=initialProject();const nodes=resolveParts(p);const x=(id:string)=>nodes.find(n=>n.part.id===id)!.position[0];expect(x('upper')).toBe(350);expect(x('booster')).toBe(1350);expect(x('lmotor')).toBe(1580);expect(boundsOf(scopeParts(nodes,'booster')).min[0]).toBe(1350)});
 it('moves nested systems and their children together',()=>{const p=initialProject();p.parts.find(n=>n.id==='avionics')!.offset=100;const nodes=resolveParts(p);expect(nodes.find(n=>n.part.id==='chute')!.position[0]).toBe(280);expect(scopeParts(nodes,'avionics').map(n=>n.part.id)).toEqual(['avionics','chute','cord'])});
 it('shares nose endpoints and handles empty assemblies',()=>{const profile=noseProfile(350,70);expect(profile[0]).toEqual([0,0]);expect(profile.at(-1)![0]).toBeCloseTo(350);expect(profile.at(-1)![1]).toBe(70);expect(boundsOf([]).max[0]).toBeGreaterThan(0)});
});
