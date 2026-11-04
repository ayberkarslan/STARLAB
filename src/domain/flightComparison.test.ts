import {describe,it,expect} from 'vitest';
import {comparisonCsv,flightComparison,flightPeaks} from './flightComparison';

describe('flight comparison',()=>{
 it('compares peaks even when sample times differ',()=>{
  const first=[{t:0,altitude:0,velocity:0,acceleration:0},{t:4,altitude:100,velocity:30,acceleration:0},{t:8,altitude:80,velocity:-12,acceleration:0}];
  const second=[{t:0,altitude:0,velocity:0,acceleration:0},{t:5,altitude:120,velocity:35,acceleration:0},{t:9,altitude:90,velocity:-14,acceleration:0}];
  expect(flightPeaks(first)).toEqual({altitude:100,apexTime:4,maxSpeed:30});
  expect(flightComparison(first,second)).toEqual({altitude:20,apexTime:1,maxSpeed:5});
 });
 it('does not invent a comparison without two flights',()=>expect(flightComparison([],[])).toBeNull());
 it('exports metrics and both original time series',()=>{
  const a={id:'starlab',samples:[{t:0,altitude:0,velocity:0,acceleration:0},{t:2,altitude:10,velocity:5,acceleration:1}]};
  const b={id:'reference',samples:[{t:0,altitude:0,velocity:0,acceleration:0},{t:3,altitude:12,velocity:7,acceleration:1}]};
  const csv=comparisonCsv(a,b);
  expect(csv).toContain('apogee_m,10,12,2');
  expect(csv).toContain('reference,3,12,7,1');
 });
});
