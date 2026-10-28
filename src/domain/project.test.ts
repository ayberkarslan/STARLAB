import {describe,it,expect} from 'vitest';
import {initialProject,validate,addPart,removePart,subtree} from './project';
import {CalculationGraph} from '../engine/graph';
describe('project integrity',()=>{
 it('round trips a two-stage project',()=>{const p=initialProject();expect(validate(JSON.parse(JSON.stringify(p)))).toEqual(p);expect(p.parts.filter(n=>n.kind==='stage')).toHaveLength(2)});
 it('rejects invalid attachment and deletes complete subtrees',()=>{const p=initialProject();expect(()=>addPart(p,'nose','stage')).toThrow();const ids=subtree(p,'sustainer').map(n=>n.id);const next=removePart(p,'sustainer');expect(next.parts.some(n=>ids.includes(n.id))).toBe(false);expect(validate(next)).toEqual(next)});
 it('rejects orphan data',()=>{const p=initialProject();p.parts[1].parentId='missing';expect(()=>validate(p)).toThrow()});
 it('recalculates only dependent modules',()=>{let a=0,b=0,c=0;const g=new CalculationGraph([{id:'a',dependencies:['x'],run:v=>{a++;return Number(v.x)*2}},{id:'b',dependencies:['a'],run:v=>{b++;return Number(v.a)+1}},{id:'c',dependencies:['y'],run:v=>{c++;return v.y}}]);g.execute({inputs:{x:1,y:2},revision:1},['x','y']);const r=g.execute({inputs:{x:3,y:2},revision:2},['x']);expect([a,b,c]).toEqual([2,2,1]);expect(r.results.b.value).toBe(7)});
});
