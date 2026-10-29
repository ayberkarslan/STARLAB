import {describe,it,expect} from 'vitest';
import {CalculationGraph,defineNode,link,valueEqual,type Contract} from './graph';
const number:Contract<number>={id:'number',unit:'1',parse:v=>{if(typeof v!=='number'||!Number.isFinite(v))throw Error('Finite number required');return v}};
const source=(id='x')=>({id,label:id,output:number});
const next=(id:string,from:string,run:(v:number)=>number)=>defineNode({id,label:id,inputs:{v:link(from,number)},output:number,run:({v})=>run(v)});
describe('incremental graph',()=>{
 it('compares JSON outputs structurally and treats unsupported objects conservatively',()=>{expect(valueEqual({a:[1,2],b:0},{b:0,a:[1,2]})).toBe(true);expect(valueEqual(Array(2),[])).toBe(false);expect(valueEqual(new Date(0),new Date(1))).toBe(false)});
 it('stops propagation at an unchanged output and ignores unrelated input changes',async()=>{
  const g=new CalculationGraph([source(),source('y')],[next('rounded','x',Math.floor),next('tail','rounded',v=>v*2),next('side','y',v=>v*3)]);
  await g.evaluate({x:1.1,y:2});const r=await g.evaluate({x:1.2,y:2});expect(r.updated).toEqual(['rounded']);expect(r.results.tail.action).toBe('cached');expect(r.results.side.runs).toBe(1);
  expect((await g.evaluate({x:1.2,y:2,notes:'changed'})).updated).toEqual([]);
 });
 it('validates wiring, cycles, units and duplicate identifiers before execution',()=>{
  expect(()=>new CalculationGraph([source()],[next('a','unknown',v=>v)])).toThrow('Unknown dependency');
  expect(()=>new CalculationGraph([],[next('a','b',v=>v),next('b','a',v=>v)])).toThrow('cycle');
  expect(()=>new CalculationGraph([source(),source()],[])).toThrow('Duplicate');
  expect(()=>new CalculationGraph([{...source(),output:{...number,unit:'m'}}],[next('a','x',v=>v)])).toThrow('Contract mismatch');
 });
 it('removes stale values after invalid input, blocks dependents and recovers',async()=>{
  const g=new CalculationGraph([source()],[next('a','x',v=>{if(v===0)throw Error('zero');return v*2}),next('b','a',v=>v+1)]);
  await g.evaluate({x:2});let r=await g.evaluate({x:0});expect(r.results.a.state.status).toBe('error');expect(r.results.b.state.status).toBe('blocked');expect('value' in r.results.b.state).toBe(false);
  r=await g.evaluate({});expect(r.results.x.state.status).toBe('missing');expect(r.results.a.state.status).toBe('blocked');
  r=await g.evaluate({x:3});expect(r.results.b.state).toEqual({status:'ready',value:7});
 });
 it('rejects invalid numeric outputs and keeps independent branches working',async()=>{
  const g=new CalculationGraph([source(),source('y')],[next('a','x',()=>NaN),next('b','a',v=>v),next('side','y',v=>v)]),r=await g.evaluate({x:1,y:8});
  expect(r.results.a.state.status).toBe('error');expect(r.results.b.state.status).toBe('blocked');expect(r.results.side.state).toEqual({status:'ready',value:8});
 });
 it('discards late asynchronous results transactionally',async()=>{
  let resolve!:(v:number)=>void;
  const asyncNode=defineNode({id:'slow',label:'slow',inputs:{v:link('x',number)},output:number,run:({v})=>v===1?new Promise<number>(r=>{resolve=r}):v});
  const g=new CalculationGraph([source()],[asyncNode]);const first=g.evaluate({x:1});const rejected=expect(first).rejects.toMatchObject({name:'AbortError'});
  const result=await g.evaluate({x:2});resolve(1);await rejected;expect(result.results.slow.state).toEqual({status:'ready',value:2});
  const cached=await g.evaluate({x:2});expect(cached.updated).toEqual([]);expect(cached.results.slow.runs).toBe(1);
 });
 it('honors cancellation and does not commit partial caches',async()=>{
  const g=new CalculationGraph([source()],[next('a','x',v=>v)]),controller=new AbortController();controller.abort();await expect(g.evaluate({x:1},controller.signal)).rejects.toMatchObject({name:'AbortError'});
  expect((await g.evaluate({x:1})).results.a.runs).toBe(1);
 });
 it('isolates input, node arguments and returned cache snapshots',async()=>{
  const object:Contract<{n:number}>={id:'object',unit:'1',parse:v=>{if(!v||typeof v!=='object'||typeof (v as {n:number}).n!=='number')throw Error('object');return v as {n:number}}};
  const g=new CalculationGraph([{id:'x',label:'X',output:object}],[defineNode({id:'a',label:'A',inputs:{v:link('x',object)},output:object,run:({v})=>{v.n++;return v}})]);
  const input={n:1},r=await g.evaluate({x:input});expect(input.n).toBe(1);(r.results.a.state as {value:{n:number}}).value.n=99;
  expect((await g.evaluate({x:{n:1}})).results.a.state).toEqual({status:'ready',value:{n:2}});
 });
});
