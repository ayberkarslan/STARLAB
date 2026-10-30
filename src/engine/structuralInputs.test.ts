import {describe,it,expect} from 'vitest';
import {structuralInputs,hasStructuralModel} from './structuralInputs';
import type {Part} from '../domain/project';
const part:Part={id:'p',parentId:'stage',kind:'innerTube',name:'İç tüp',length:300,diameter:33,offset:0,mass:0,material:'Tanımlanmadı'};
describe('structural SI boundary',()=>{
 it('converts explicit inner diameter and density without defaults',()=>{
  expect(structuralInputs({...part,geometry:{innerDiameter:30},inputs:{density:1500}})).toEqual({model:'inner-tube',inputs:{length:.3,radius:.0165,innerRadius:.015,density:1500},issue:undefined});
  expect(structuralInputs(part).inputs.innerRadius).toBeUndefined();
  expect(structuralInputs(part).inputs.density).toBeUndefined();
 });
 it('converts a provided wall and reports conflicting dimensions',()=>{
  expect(structuralInputs({...part,inputs:{wallThickness:1.5}}).inputs.innerRadius).toBe(.015);
  expect(structuralInputs({...part,geometry:{innerDiameter:30},inputs:{wallThickness:2}}).issue).toBeTruthy();
 });
 it('keeps missing, disabled, and explicit zero supplied masses distinct',()=>{
  const mass={...part,kind:'mass' as const,mass:300};
  expect(structuralInputs(mass).inputs.suppliedMass).toBe(.3);
  expect(structuralInputs({...mass,inputs:{massProvided:false}}).inputs.suppliedMass).toBeUndefined();
  expect(structuralInputs({...mass,mass:0}).inputs.suppliedMass).toBeUndefined();
  expect(structuralInputs({...mass,mass:0,inputs:{massProvided:true}}).inputs.suppliedMass).toBe(0);
 });
 it('never maps an unsupported component to a cylinder model',()=>{
  expect(hasStructuralModel({...part,kind:'freeformFins'})).toBe(false);
  expect(()=>structuralInputs({...part,kind:'freeformFins'})).toThrow('henüz bağlı değil');
 });
});


describe('production structural adapters',()=>{
 it('requires an explicit nose profile and structure, supports a solid nose without inventing density',()=>{
  const nose={...part,kind:'nose' as const};
  expect(structuralInputs(nose).issue).toBeTruthy();
  const request=structuralInputs({...nose,geometry:{noseProfile:'conical',filled:true}});
  expect(request.issue).toBeUndefined();expect(request.inputs.wall).toBe(0);expect(request.inputs.density).toBeUndefined();
  expect(structuralInputs({...nose,geometry:{noseProfile:'secantOgive',filled:false}}).issue).toBeTruthy();
 });
 it('preserves nose shoulder/cap units and unknown shoulder data',()=>{
  const nose={...part,kind:'nose' as const,geometry:{noseProfile:'tangentOgive' as const,filled:false,shoulderLength:20,shoulderDiameter:30,shoulderThickness:2,shoulderCapped:true}};
  expect(structuralInputs(nose).inputs).toMatchObject({shoulderLength:.02,shoulderRadius:.015,shoulderWall:.002,capped:1});
  expect(structuralInputs({...nose,geometry:{...nose.geometry,shoulderThickness:undefined}}).inputs.shoulderWall).toBeUndefined();
 });
 it('projects fin geometry from the actual parent and does not reuse root chord as axial extent',()=>{
  const parent={...part,id:'body',kind:'body' as const,diameter:60};
  const fin={...part,kind:'fins' as const,parentId:'body',length:200,diameter:300,geometry:{tipChord:100,sweep:150,finCount:3,finCrossSection:'rounded' as const}};
  const project={version:1 as const,name:'Test',parts:[parent,fin]};
  const request=structuralInputs(fin,project);expect(request.issue).toBeUndefined();
  expect(request.inputs).toMatchObject({root:.2,tip:.1,span:.12,sweep:.15,bodyRadius:.03,count:3,sectionCode:1});
  expect(structuralInputs({...fin,geometry:{...fin.geometry,cantAngle:1}},project).issue).toBeTruthy();
  expect(structuralInputs({...fin,geometry:{...fin.geometry,span:100}},project).issue).toBeTruthy();
 });
 it('keeps a missing ring hole unknown and gives a bulkhead its explicit solid topology',()=>{
  expect(structuralInputs({...part,kind:'ring'}).inputs.innerRadius).toBeUndefined();
  expect(structuralInputs({...part,kind:'bulkhead'}).inputs.innerRadius).toBe(0);
 });
});

it('preserves the mass-affecting shoulder cap through a project file round trip',async()=>{
 const {initialProject}=await import('../domain/project');
 const {serializeStar,parseStar}=await import('../domain/starFile');
 const project=initialProject();const nose=project.parts.find(p=>p.kind==='nose')!;
 nose.geometry={noseProfile:'tangentOgive',filled:false,shoulderLength:20,shoulderDiameter:30,shoulderThickness:2,shoulderCapped:true};
 const restored=parseStar(serializeStar(project)).parts.find(p=>p.id===nose.id)!;
 expect(restored.geometry?.shoulderCapped).toBe(true);
 expect(structuralInputs(restored).inputs.capped).toBe(1);
});
