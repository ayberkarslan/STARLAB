import {describe,it,expect} from 'vitest';
import {geometryPatch,editGeometry} from './geometryEdit';
import {initialProject,validate} from './project';
describe('direct geometry input',()=>{
 it('snaps coordinates and keeps thin shell dimensions valid',()=>{
  const p=initialProject(),part=p.parts.find(p=>p.id==='upper')!;part.inputs={wallThickness:4};
  expect(geometryPatch(part,'move',-12.4)).toEqual({offset:-12});expect(geometryPatch(part,'move',.26,.1)).toEqual({offset:.3});
  expect(geometryPatch(part,'diameter',-200)).toEqual({diameter:8});expect(geometryPatch(part,'length',-2000)).toEqual({length:1});
 });
 it('changes only the requested part and preserves its children and data',()=>{
  const p=initialProject(),part=p.parts.find(p=>p.id==='upper')!;part.inputs={centerOfMass:200};
  const edited=editGeometry(p,part.id,{length:1200,offset:20});expect(validate(edited)).toEqual(edited);
  expect(edited.parts.find(p=>p.id==='umotor')).toBe(p.parts.find(p=>p.id==='umotor'));expect(edited.parts.find(p=>p.id===part.id)?.inputs).toBe(part.inputs);
  expect(editGeometry(p,part.id,{length:part.length})).toBe(p);expect(()=>editGeometry(p,part.id,{diameter:NaN})).toThrow();
 });
});
