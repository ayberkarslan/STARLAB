import {describe,it,expect} from 'vitest';
import {snapCandidates,snapGeometry} from './geometrySnap';
import {initialProject} from './project';
describe('screen scale alignment snapping',()=>{
 it('matches body diameters across stages with a sticky release range',()=>{
  const p=initialProject(),part=p.parts.find(p=>p.id==='upper')!;const candidates=snapCandidates(p,part,'diameter');
  expect(candidates.some(c=>c.targetId==='lower'&&c.value===140)).toBe(true);
  const enter=snapGeometry({diameter:150},candidates,1);expect(enter.patch.diameter).toBe(140);
  const hold=snapGeometry({diameter:160},candidates,1,enter.snap);expect(hold.patch.diameter).toBe(140);
  expect(snapGeometry({diameter:165},candidates,1,hold.snap).snap).toBeNull();
  expect(snapGeometry({diameter:150},candidates,.1).snap).toBeNull();
 });
 it('aligns axial edges while excluding descendants as moving targets',()=>{
  const p=initialProject(),part=p.parts.find(p=>p.id==='upper')!;const candidates=snapCandidates(p,part,'move');
  expect(candidates.some(c=>c.targetId==='lower')).toBe(true);expect(candidates.some(c=>c.targetId==='umotor')).toBe(false);
  expect(candidates.some(c=>c.value===0)).toBe(true);
 });
});
