import {describe,it,expect} from 'vitest';
import {initialProject,validate,addPart,removePart,subtree} from './project';
import {CalculationGraph,defineNode,link,type Contract} from '../engine/graph';
describe('project integrity',()=>{
 it('round trips a two-stage project',()=>{const p=initialProject();expect(validate(JSON.parse(JSON.stringify(p)))).toEqual(p);expect(p.parts.filter(n=>n.kind==='stage')).toHaveLength(2)});
 it('rejects invalid attachment and deletes complete subtrees',()=>{const p=initialProject();expect(()=>addPart(p,'nose','stage')).toThrow();const ids=subtree(p,'sustainer').map(n=>n.id);const next=removePart(p,'sustainer');expect(next.parts.some(n=>ids.includes(n.id))).toBe(false);expect(validate(next)).toEqual(next)});
 it('rejects orphan data',()=>{const p=initialProject();p.parts[1].parentId='missing';expect(()=>validate(p)).toThrow()});
 it('recalculates only dependent modules',async()=>{
  const number:Contract<number>={id:'number',unit:'1',parse:v=>{if(typeof v!=='number')throw Error('number');return v}};
  let a=0,b=0,c=0;const g=new CalculationGraph([{id:'x',label:'X',output:number},{id:'y',label:'Y',output:number}],[
   defineNode({id:'a',label:'A',inputs:{x:link('x',number)},output:number,run:v=>{a++;return v.x*2}}),
   defineNode({id:'b',label:'B',inputs:{a:link('a',number)},output:number,run:v=>{b++;return v.a+1}}),
   defineNode({id:'c',label:'C',inputs:{y:link('y',number)},output:number,run:v=>{c++;return v.y}}),
  ]);
  await g.evaluate({x:1,y:2});const r=await g.evaluate({x:3,y:2});expect([a,b,c]).toEqual([2,2,1]);expect(r.results.b.state).toEqual({status:'ready',value:7});
 });
});
