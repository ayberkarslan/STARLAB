import {describe,it,expect} from 'vitest';
import {initialProject,children,validate} from './project';
import {movePart,moveError} from './move';
describe('hierarchy moves',()=>{
 it('moves a complete subsystem to another stage without changing local offsets',()=>{const p=initialProject();const next=movePart(p,'avionics',{id:'booster',placement:'inside'});expect(next.parts.find(n=>n.id==='avionics')!.parentId).toBe('booster');expect(next.parts.find(n=>n.id==='chute')).toEqual(p.parts.find(n=>n.id==='chute'));expect(validate(next)).toEqual(next);expect(p.parts.find(n=>n.id==='avionics')!.parentId).toBe('sustainer')});
 it('reorders siblings before and after',()=>{let p=initialProject();p=movePart(p,'booster',{id:'sustainer',placement:'before'});expect(children(p,'rocket').map(p=>p.id)).toEqual(['booster','sustainer']);p=movePart(p,'booster',{id:'sustainer',placement:'after'});expect(children(p,'rocket').map(p=>p.id)).toEqual(['sustainer','booster'])});
 it('moves a fin set to another compatible body',()=>{const p=movePart(initialProject(),'ufins',{id:'lower',placement:'inside'});expect(children(p,'lower').map(n=>n.id)).toContain('ufins');expect(validate(JSON.parse(JSON.stringify(p)))).toEqual(p)});
 it('rejects root, self, descendant and incompatible drops',()=>{const p=initialProject();for(const [source,id,placement] of [['rocket','booster','inside'],['upper','upper','inside'],['upper','ufins','before'],['ufins','avionics','inside'],['booster','sustainer','inside']] as const){expect(moveError(p,source,{id,placement})).toBeTruthy();expect(()=>movePart(p,source,{id,placement})).toThrow()}});
 it('rejects cycles between assemblies',()=>{let p=initialProject();p.parts.push({...p.parts.find(n=>n.id==='avionics')!,id:'nested',parentId:'avionics'});expect(()=>movePart(p,'avionics',{id:'nested',placement:'inside'})).toThrow()});
});
