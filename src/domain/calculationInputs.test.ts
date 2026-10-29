import {describe,it,expect} from 'vitest';
import {initialProject,validate} from './project';
import {patchSelection} from './selection';
import {parseStar,serializeStar} from './starFile';
import {createProjectInputs} from '../engine/projectInputs';
import {editGeometry} from './geometryEdit';
import {initialFlight} from './flight';

describe('calculation input collection',()=>{
 it('keeps explicit shell choices and fin parameters through project files and packets',()=>{
  const p=initialProject(),nose=p.parts.find(p=>p.id==='nose')!,fin=p.parts.find(p=>p.id==='ufins')!;
  p.flight=initialFlight();
  nose.geometry={filled:false};
  fin.geometry={finCount:4,finCrossSection:'diamond',tabHeight:12,tabLength:80};
  fin.inputs={wallThickness:3,shearModulus:26e9,normalForceSlope:0};
  const restored=parseStar(serializeStar(p));expect(restored).toEqual(p);
  const packet=createProjectInputs(restored),input=packet.project.parts.find(p=>p.id===fin.id)!;
  expect(packet.project.parts.find(p=>p.id==='nose')!.geometry?.filled).toBe(false);
  expect(input.geometry).toEqual(fin.geometry);
  expect(input.inputs.shearModulus).toBe(26e9);expect(input.inputs.normalForceSlope).toBe(0);
  expect(packet.units.parts.shearModulus).toBe('Pa');expect(packet.units.parts.normalForceSlope).toBe('1/rad');
  expect(packet.units.geometry.tabHeight).toBe('mm');expect(input.mass).toBeNull();
 });
 it('preserves unknown inputs and never substitutes a catalog modulus after clearing',()=>{
  const p=initialProject(),fin=p.parts.find(p=>p.id==='ufins')!;
  fin.materialData={key:'test',name:'Test',type:'bulk',density:2700,shearModulus:26e9,group:'Test'};
  const input=createProjectInputs(p).project.parts.find(p=>p.id===fin.id)!;
  expect(input.inputs.shearModulus).toBeNull();expect(input.inputs.normalForceSlope).toBeNull();
  expect(input.materialData?.shearModulus).toBe(26e9);expect(input.geometry?.finCrossSection).toBeUndefined();
 });
 it('patches selected geometry independently without replacing different fin layouts',()=>{
  const p=initialProject(),a=p.parts.find(p=>p.id==='ufins')!,b=p.parts.find(p=>p.id==='lfins')!;
  a.geometry={finCount:3,tipChord:70,finCrossSection:'square'};b.geometry={finCount:4,tipChord:90,tabHeight:5};
  a.inputs={density:2700};b.inputs={wallThickness:4};
  const next=patchSelection(p,[a.id,b.id],{geometry:{finCrossSection:'rounded',tabLength:60},inputs:{shearModulus:10e9}});
  expect(next.parts.find(p=>p.id===a.id)!.geometry).toEqual({...a.geometry,finCrossSection:'rounded',tabLength:60});
  expect(next.parts.find(p=>p.id===b.id)!.geometry).toEqual({...b.geometry,finCrossSection:'rounded',tabLength:60});
  expect(next.parts.find(p=>p.id===a.id)!.inputs?.density).toBe(2700);
  expect(b.geometry.tabLength).toBeUndefined();expect(b.inputs.shearModulus).toBeUndefined();
 });
 it('rejects malformed new inputs atomically while allowing signed CNa',()=>{
  const p=initialProject();
  for(const geometry of [{filled:'false'},{finCrossSection:'invented'},{tabHeight:-1},{tabLength:NaN}]){
   expect(()=>patchSelection(p,['ufins'],{geometry:geometry as never})).toThrow();
  }
  for(const inputs of [{shearModulus:0},{shearModulus:-1},{shearModulus:Infinity},{normalForceSlope:'1'}]){
   expect(()=>patchSelection(p,['ufins'],{inputs:inputs as never})).toThrow();
  }
  expect(()=>validate(p)).not.toThrow();
  expect(patchSelection(p,['ufins'],{inputs:{normalForceSlope:-2}}).parts.find(p=>p.id==='ufins')!.inputs?.normalForceSlope).toBe(-2);
 });
 it('scales axial tab length with a canvas stretch and retains its independent height',()=>{
  const p=initialProject(),fin=p.parts.find(p=>p.id==='ufins')!;fin.geometry={tabLength:80,tabHeight:12,finCrossSection:'square'};
  const next=editGeometry(p,fin.id,{length:fin.length*2}).parts.find(p=>p.id===fin.id)!;
  expect(next.geometry).toEqual({tabLength:160,tabHeight:12,finCrossSection:'square'});
 });
});
