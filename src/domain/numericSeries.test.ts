import {describe,it,expect} from 'vitest';
import {parseNumericRows,parseSeriesCsv} from './numericSeries';
import {motorMassColumns} from './motorMass';
import {windColumns} from './environmentInputs';
describe('numeric profile input parsing',()=>{
 it('keeps optional blanks unknown and preserves explicit zero and decimal commas',()=>{
  expect(parseNumericRows([['0','2','1','','0','','']],motorMassColumns)[0]).toEqual({t:0,totalMass:2,propellantMass:1,centerOfMass:undefined,inertiaXX:0,inertiaYY:undefined,inertiaZZ:undefined});
  expect(parseNumericRows([['0','1,5','360']],windColumns)[0].speed).toBe(1.5);
 });
 it('uses an explicit CSV header and rejects empty required cells and nondecimal values',()=>{
  expect(parseSeriesCsv('\ufeffaltitude,speed,direction\r\n0,0,359\r\n1000,10,1',windColumns)).toHaveLength(2);
  for(const csv of ['speed,altitude,direction\n0,0,0','altitude,speed,direction\n0,,0','altitude,speed,direction\n0,NaN,0','altitude,speed,direction\n0,0x10,0','altitude,speed,direction\n0,0,361'])expect(()=>parseSeriesCsv(csv,windColumns)).toThrow();
 });
 it('limits profile size and accepts a header-only file for explicit clearing',()=>{
  expect(()=>parseNumericRows(Array.from({length:10001},()=>['0','0','0']),windColumns)).toThrow();
  expect(parseSeriesCsv('altitude,speed,direction\n',windColumns)).toEqual([]);
 });
});
