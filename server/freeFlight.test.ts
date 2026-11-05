import {describe,it,expect} from 'vitest';
import {runFreeFlight} from './freeFlight';
import {parseFreeFlightHistory} from '../src/engine/freeFlightSolver';

const parts=[
 'cone 0 0 .36 0 .07 0 0 0 0 0 0',
 'tube 1 .36 .55 .07 .07 0 0 0 0 0 0',
 'transition 2 .91 .12 .07 .08 0 0 0 0 0 0',
 'boattail 3 1.03 .12 .08 .05 0 0 0 0 0 0',
 'fins 4 .92 .16 .08 0 4 .09 .16 .07 .06 .002',
];
function input(eastWind=0){return `STARLAB_FREE_FLIGHT_V1\n0.02 980 15 50 20 0 1 ${eastWind} 0 0.35 0 0.15 10 10 -0.7 0.1 2 2 0.1 2 2 0.16 0.001 boattail\n2\n0 120\n0.15 0\n5\n${parts.join('\n')}\n`}

describe('supplied free flight model process',()=>{
 it('runs both DoF modes and preserves geometry telemetry',async()=>{
  for(const mode of ['3','6'] as const){
   const raw=await runFreeFlight(mode,input(),new AbortController().signal);
   const result=parseFreeFlightHistory(raw.history,raw.components);
   expect(result.samples.length).toBeGreaterThan(10);
   expect(result.samples[0].altitude).toBeCloseTo(15);
   expect(result.samples.at(-1)?.t).toBeCloseTo(.35);
   expect(result.samples[0].mach).toBeGreaterThan(0);
   expect(result.components.map(item=>item.id)).toEqual(['cone','tube','transition','boattail','fins']);
  }
 });
 it('rejects lateral wind in the planar model',async()=>{
  await expect(runFreeFlight('3',input(2),new AbortController().signal)).rejects.toThrow();
  const raw=await runFreeFlight('6',input(2),new AbortController().signal);
  expect(parseFreeFlightHistory(raw.history,raw.components).samples.at(-1)?.east).not.toBe(0);
 });
 it('accepts the supplied solver initial body state in 6DOF',async()=>{
  const base=input(),v2=base.replace('STARLAB_FREE_FLIGHT_V1','STARLAB_FREE_FLIGHT_V2').replace('boattail\n2\n','boattail\n1 4 50 2 -1 .01 .02 .03\n2\n');
  const raw=await runFreeFlight('6',v2,new AbortController().signal);
  const columns=raw.history.trim().split(/\r?\n/),names=columns[0].split(','),first=columns[1].split(',');
  for(const [key,value] of Object.entries({u:50,v:2,w:-1,p:.01,q:.02,r:.03}))expect(Number(first[names.indexOf(key)])).toBeCloseTo(value);
  expect(Number(first[names.indexOf('k1')])).not.toBe(0);
  await expect(runFreeFlight('3',v2,new AbortController().signal)).rejects.toThrow('outside 3DOF plane');
 });
 it('matches the supplied C++ geometry fixture at the final step',async()=>{
  const source=input().replace('0.15 0\n5\n','0.15 120\n5\n');
  const raw=await runFreeFlight('6',source,new AbortController().signal);
  const rows=raw.history.trim().split(/\r?\n/),header=rows[0].split(','),last=rows.at(-1)!.split(',');
  const value=(name:string)=>Number(last[header.indexOf(name)]);
  expect(rows).toHaveLength(20);
  expect(value('n_m')).toBeCloseTo(6.1484276289014,10);
  expect(value('down_m')).toBeCloseTo(-31.269841192171,10);
  expect(value('Mach')).toBeCloseTo(.14272698757132,10);
  expect(value('CP_m')).toBeCloseTo(.66519179971344,10);
  expect(value('CD')).toBeCloseTo(.23460182781791,10);
 });
});
