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
function input(eastWind=0){return `0.02 980 15 50 20 0 1 ${eastWind} 0 0.35 0 0.15 10 10 -0.7 0.1 2 2 0.1 2 2 0.16 0.001 boattail\n2\n0 120\n0.15 0\n5\n${parts.join('\n')}\n`}

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
});
