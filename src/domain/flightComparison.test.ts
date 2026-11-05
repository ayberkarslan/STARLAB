import {describe,it,expect} from 'vitest';
import {validateCoefficientTable} from './aerodynamics';
import {computeFinFlutterDiagnostics,generateMachAeroTable,machAeroTableToRasaeroGrid} from './aeroSweep';
import {comparisonCsv,flightComparison,flightPeaks,seriesRmseAndSimilarity} from './flightComparison';
import {initialProject} from './project';

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
  expect(csv).toContain('altitude_rmse_m');
  expect(csv).toContain('gaussian_similarity_pct');
  expect(csv).toContain('reference,3,12,7,1');
 });
 it('computes RMSE and Gaussian similarity score and Mach 0.3-2.0 aero table',()=>{
  const a=[{t:0,altitude:0,velocity:0,acceleration:0},{t:10,altitude:1000,velocity:150,acceleration:0},{t:20,altitude:1770,velocity:0,acceleration:-9.8}];
  const b=[{t:0,altitude:0,velocity:0,acceleration:0},{t:10,altitude:1002,velocity:150,acceleration:0},{t:20,altitude:1772,velocity:0,acceleration:-9.8}];
  const stats=seriesRmseAndSimilarity(a,b,'altitude');
  expect(stats).not.toBeNull();
  expect(stats!.rmse).toBeLessThan(3);
  expect(stats!.gaussianSimilarityPct).toBeGreaterThan(99);
  const project=initialProject();
  const machTable=generateMachAeroTable(project,1100,4);
  expect(machTable[0].mach).toBe(0.3);
  expect(machTable.at(-1)?.mach).toBe(2);
  expect(machTable.every(r=>r.cdOff>r.cdOn&&r.cpMm>0)).toBe(true);
  const rasaeroGrid=machAeroTableToRasaeroGrid(project,1100);
  expect(()=>validateCoefficientTable(rasaeroGrid)).not.toThrow();
  const flutter=computeFinFlutterDiagnostics(project,250);
  expect(flutter.length).toBeGreaterThan(0);
  expect(flutter[0].flutterSpeedMs).toBeGreaterThan(100);
 });
});
