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
  expect(hasStructuralModel({...part,kind:'nose'})).toBe(false);
  expect(()=>structuralInputs({...part,kind:'nose'})).toThrow('henüz bağlı değil');
 });
});
