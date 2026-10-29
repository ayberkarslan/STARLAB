import {describe,it,expect} from 'vitest';
import {cinematicPose} from './cinematicCamera';
const bounds={min:[0,-150,-150] as [number,number,number],max:[2200,150,150] as [number,number,number]};
describe('cinematic camera path',()=>{
 it('stays finite and outside the model across shots and aspect ratios',()=>{
  for(const aspect of [.3,1,16/9,3])for(let t=0;t<=96;t+=.25){const p=cinematicPose(bounds,aspect,t);expect([...p.position,...p.target,p.near,p.far].every(Number.isFinite)).toBe(true);expect(Math.hypot(p.position[1],p.position[2])).toBeGreaterThan(.15);expect(p.near).toBeGreaterThan(0);expect(p.far).toBeGreaterThan(p.near)}
 });
 it('loops continuously without jumping from one shot to the next',()=>{
  for(const t of [12,24,36,48]){const before=cinematicPose(bounds,1.6,t-.0001),after=cinematicPose(bounds,1.6,t+.0001);expect(Math.hypot(...before.position.map((v,i)=>v-after.position[i]))).toBeLessThan(.001)}
  expect(cinematicPose(bounds,1.6,48).position).toEqual(cinematicPose(bounds,1.6,0).position.map((v,i)=>expect.closeTo(v,10)));
 });
});
