import {describe,it,expect} from 'vitest';
import {initialProject,emptyProject} from '../domain/project';
import {initialFlight} from '../domain/flight';
import {inputReadiness} from './inputReadiness';
describe('basic input presence checks',()=>{
 it('shows missing fields without asking for derived CG/CP/Cd/CNa results',()=>{
  const p=initialProject(),groups=inputReadiness(p,initialFlight());
  expect(groups.find(g=>g.id==='geometry')!.gaps.some(g=>g.partId==='nose'&&g.fields.includes('Burun yapısı'))).toBe(true);
  expect(groups.flatMap(g=>g.gaps.flatMap(g=>g.fields)).join(' ')).not.toMatch(/CG|CP|CNa/);
  p.aerodynamics={referenceDiameter:140,mach:0,angleOfAttack:0,motorOn:false};expect(inputReadiness(p,initialFlight()).find(g=>g.id==='aero')!.gaps).toEqual([]);
 });
 it('finds empty profile selections and disabled recovery needs no open canopy inputs',()=>{
  const p=initialProject(),f=initialFlight();f.cases[0].environment.atmosphereModel='profile';f.cases[0].environment.windModel='profile';
  f.configurations[0].recovery.chute={event:'never',delay:0,altitude:300};
  const groups=inputReadiness(p,f);expect(groups.find(g=>g.id==='environment')!.gaps[0].fields).toEqual(['Atmosfer profili','Rüzgâr profili']);
  expect(groups.find(g=>g.id==='recovery')!.gaps.some(g=>g.partId==='chute')).toBe(false);
  expect(inputReadiness(emptyProject(),f).find(g=>g.id==='geometry')!.gaps[0].fields).toEqual(['Bileşen ekle']);
 });
 it('requires measured mass for a mass component even when a material density is present',()=>{
  const p=initialProject();p.parts.push({id:'ballast',parentId:'upper',kind:'mass',name:'Ballast',length:10,diameter:10,offset:0,mass:0,material:'Test',inputs:{density:2700}});
  expect(inputReadiness(p,initialFlight())[0].gaps.find(g=>g.partId==='ballast')!.fields).toEqual(['Kütle']);
 });
});
