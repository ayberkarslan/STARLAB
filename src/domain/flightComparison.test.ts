import {describe,it,expect} from 'vitest';
import {flightComparison,flightPeaks} from './flightComparison';

describe('flight comparison',()=>{
 it('compares peaks even when sample times differ',()=>{
  const first=[{t:0,altitude:0,velocity:0,acceleration:0},{t:4,altitude:100,velocity:30,acceleration:0},{t:8,altitude:80,velocity:-12,acceleration:0}];
  const second=[{t:0,altitude:0,velocity:0,acceleration:0},{t:5,altitude:120,velocity:35,acceleration:0},{t:9,altitude:90,velocity:-14,acceleration:0}];
  expect(flightPeaks(first)).toEqual({altitude:100,apexTime:4,maxSpeed:30});
  expect(flightComparison(first,second)).toEqual({altitude:20,apexTime:1,maxSpeed:5});
 });
 it('does not invent a comparison without two flights',()=>expect(flightComparison([],[])).toBeNull());
});
