import {readFileSync} from 'node:fs';
import {beforeAll,afterAll,describe,it,expect,vi} from 'vitest';
import {createTubeGraph,tubeInputs,tubeNodes} from './index';
import {tubeSources,mass} from './contracts';
import {CalculationGraph,defineNode,link} from '../graph';
import {initialProject} from '../../domain/project';
beforeAll(()=>{const bytes=readFileSync(new URL('../../../public/engines/tube-v1.wasm',import.meta.url));vi.spyOn(globalThis,'fetch').mockImplementation(async()=>new Response(new Uint8Array(bytes).buffer))});
afterAll(()=>vi.restoreAllMocks());
const values={length:1,radius:.03,wall:.002,density:2700};
const get=(r:Awaited<ReturnType<CalculationGraph['evaluate']>>,id:string)=>{expect(r.results[id].state.status).toBe('ready');return (r.results[id].state as {value:number}).value};
describe('native tube module example',()=>{
 it('runs the bundled C++ module and maps SI units and axial inertia explicitly',async()=>{
  const r=await createTubeGraph().evaluate(values),v=Math.PI*(.03**2-.028**2),m=2700*v;
  expect(get(r,'volume')).toBeCloseTo(v,14);expect(get(r,'mass')).toBeCloseTo(m,12);expect(get(r,'cg')).toBe(.5);
  expect(get(r,'axial')).toBeCloseTo(.5*m*(.03**2+.028**2),14);expect(get(r,'transverse')).toBeCloseTo(m/12*(3*(.03**2+.028**2)+1),12);
 });
 it('recomputes only mass and inertia for a density edit; a zero-mass output stops its chain',async()=>{
  const g=createTubeGraph();await g.evaluate(values);let r=await g.evaluate({...values,density:1500});expect(new Set(r.updated)).toEqual(new Set(['mass','axial','transverse']));expect(r.results.cg.runs).toBe(1);expect(r.results.volume.runs).toBe(1);
  await g.evaluate({...values,wall:0});r=await g.evaluate({...values,wall:0,density:1500});expect(r.updated).toEqual(['mass']);expect(get(r,'mass')).toBe(0);
 });
 it('does not substitute catalog data for missing user input or pass invalid geometry',async()=>{
  const p=initialProject().parts.find(p=>p.id==='upper')!;p.inputs={wallThickness:2};const inputs=tubeInputs(p);expect(inputs.length).toBe(1);expect(inputs.radius).toBe(.07);expect(inputs.wall).toBe(.002);expect(inputs.density).toBeUndefined();
  const g=createTubeGraph();let r=await g.evaluate(inputs);expect(r.results.mass.state.status).toBe('blocked');expect(r.results.cg.state.status).toBe('ready');
  r=await g.evaluate({...values,wall:.03});expect(r.results.volume.state.status).toBe('error');expect(r.results.mass.state.status).toBe('blocked');
 });
 it('accepts a new downstream node without changing the scheduler, old nodes or diagram contract',async()=>{
  const extra=defineNode({id:'double-mass',label:'Extension example',inputs:{mass:link('mass',mass)},output:mass,run:({mass})=>mass*2});
  const r=await new CalculationGraph(tubeSources,[...tubeNodes,extra]).evaluate(values);expect(get(r,'double-mass')).toBe(get(r,'mass')*2);
 });
});
