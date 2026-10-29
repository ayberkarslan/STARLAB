import {describe,it,expect} from 'vitest';
import {cinematicPose,CINEMATIC_SHOT_SECONDS} from './cinematicCamera';
const bounds={min:[0,-150,-150] as [number,number,number],max:[2200,150,150] as [number,number,number]};
describe('cinematic camera path',()=>{
 it('stays finite and outside the model across shots and aspect ratios',()=>{
  for(const aspect of [.3,1,16/9,3])for(let t=0;t<=96;t+=.25){const p=cinematicPose(bounds,aspect,t);expect([...p.position,...p.target,p.near,p.far].every(Number.isFinite)).toBe(true);expect(Math.hypot(p.position[1],p.position[2])).toBeGreaterThan(.15);expect(p.near).toBeGreaterThan(0);expect(p.far).toBeGreaterThan(p.near)}
 });
 it('moves slowly within each shot and cuts directly to the next framing',()=>{
  for(let shot=0;shot<4;shot++){
   const t=shot*CINEMATIC_SHOT_SECONDS+5,before=cinematicPose(bounds,1.6,t),after=cinematicPose(bounds,1.6,t+.01);
   const drift=Math.hypot(...before.position.map((v,i)=>v-after.position[i]));expect(drift).toBeGreaterThan(0);expect(drift).toBeLessThan(.01);
   const cut=(shot+1)*CINEMATIC_SHOT_SECONDS,a=cinematicPose(bounds,1.6,cut-.0001),b=cinematicPose(bounds,1.6,cut);
   expect(Math.hypot(...a.position.map((v,i)=>v-b.position[i]))).toBeGreaterThan(.5);
  }
  expect(cinematicPose(bounds,1.6,4*CINEMATIC_SHOT_SECONDS)).toEqual(cinematicPose(bounds,1.6,0));
 });
});
