import {describe,it,expect} from 'vitest';
import {initialProject,validate} from './project';
describe('user component inputs',()=>{
 it('preserves unknowns and explicit zero without computing centers',()=>{
  const p=initialProject();p.parts[0].mass=12400;p.parts[0].inputs={massProvided:true,massScope:'subtree',centerOfMass:0,centerOfPressure:1800,mach:.8,angleOfAttack:3,notes:'ölçüm'};
  p.parts[3].inputs={wallThickness:2,density:1450};
  const restored=validate(JSON.parse(JSON.stringify(p)));expect(restored).toEqual(p);expect(restored.parts[3].inputs?.centerOfMass).toBeUndefined();
 });
 it('rejects impossible thickness and malformed input values',()=>{
  const p=initialProject();p.parts[3].inputs={wallThickness:71};expect(()=>validate(p)).toThrow();
  for(const inputs of [{density:0},{centerOfMass:null},{mach:-1},{massProvided:'true'},{angleOfAttack:181}]){
   p.parts[3].inputs=inputs as never;expect(()=>validate(p)).toThrow();
  }
 });
});
