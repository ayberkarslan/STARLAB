import {describe,it,expect} from 'vitest';
import {initialProject} from './project';
import {initialFlight,simulationSignature} from './flight';
import {sampleMotors} from './motors';
import {parseStar,serializeStar,starFilename} from './starFile';
describe('star project files',()=>{
 it('round trips the complete workspace and user inputs',()=>{
  const p=initialProject();p.parts[0].inputs={centerOfMass:450,centerOfPressure:620,notes:'test'};p.flight=initialFlight();
  p.flight.cases[0].result={source:'imported',signature:'external',samples:[{t:0,altitude:0,velocity:0,acceleration:0},{t:1,altitude:20,velocity:30,acceleration:5}]};
  expect(parseStar(serializeStar(p))).toEqual(p);
 });
 it('embeds referenced sample motors and rewires assignments',()=>{
  const p=initialProject();p.flight=initialFlight();p.flight.configurations[0].mounts.umotor={motorId:'sample-54',event:'launch',delay:0};
  const restored=parseStar(serializeStar(p));const mount=restored.flight!.configurations[0].mounts.umotor;
  expect(restored.flight!.motors.some(m=>m.id===mount.motorId)).toBe(true);
  expect(parseStar(serializeStar(restored))).toEqual(restored);
 });
 it('accepts legacy json, rejects broken or future files',()=>{
  const p=initialProject();expect(parseStar(JSON.stringify(p))).toEqual(p);
  expect(()=>parseStar('{')).toThrow();expect(()=>parseStar('{"format":"starlab","schemaVersion":2}')).toThrow();
  const broken=JSON.parse(serializeStar(p));broken.project.parts[1].parentId='missing';expect(()=>parseStar(JSON.stringify(broken))).toThrow();
  expect(starFilename('test/a:b')).toBe('test-a-b.star');
 });
 it('avoids motor id collisions and preserves result freshness',()=>{
  const p=initialProject();const f=p.flight=initialFlight();const c=f.configurations[0];c.mounts.umotor={motorId:'sample-54',event:'launch',delay:0};
  f.motors.push({...sampleMotors[0],id:'bundled-sample-54',source:'custom'});
  f.cases[0].result={source:'imported',signature:simulationSignature(p,c,f.cases[0].environment,f),samples:[{t:0,altitude:0,velocity:0,acceleration:0},{t:1,altitude:1,velocity:1,acceleration:1}]};
  const restored=parseStar(serializeStar(p)),flight=restored.flight!;
  expect(flight.configurations[0].mounts.umotor.motorId).toBe('bundled-sample-54-copy');
  expect(flight.cases[0].result?.signature).toBe(simulationSignature(restored,flight.configurations[0],flight.cases[0].environment,flight));
  f.cases[0].result.signature='stale';expect(parseStar(serializeStar(p)).flight!.cases[0].result?.signature).toBe('stale');
 });
});
