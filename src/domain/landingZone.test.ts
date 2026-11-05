import {describe,expect,it} from 'vitest';
import {computeLandingEllipse,defaultDispersionSettings,landingZoneCsv,singleFlightLandingZone} from './landingZone';
import {emptyProject} from './project';
import {initialFlight} from './flight';
import {parseStar,serializeStar} from './starFile';

describe('landing zone domain math',()=>{
 it('computes 1sigma, 2sigma, 3sigma ellipse axes and orientation from impact points',()=>{
  const points=[
   {east:100,north:200,nominal:true},
   {east:140,north:200},
   {east:60,north:200},
   {east:100,north:220},
   {east:100,north:180},
  ];
  const ellipse=computeLandingEllipse(points);
  expect(ellipse.centerEast).toBeCloseTo(100,6);
  expect(ellipse.centerNorth).toBeCloseTo(200,6);
  expect(ellipse.semiMajor1Sigma).toBeCloseTo(Math.sqrt(800),5);
  expect(ellipse.semiMinor1Sigma).toBeCloseTo(Math.sqrt(200),5);
  expect(ellipse.semiMajor3Sigma).toBeCloseTo(3*Math.sqrt(800),5);
  expect(ellipse.semiMinor3Sigma).toBeCloseTo(3*Math.sqrt(200),5);
  expect(Math.abs(ellipse.angleDeg)).toBeCloseTo(0,5);
  expect(ellipse.cep50Radius).toBeGreaterThan(20);
 });

 it('builds a single-flight 3D landing zone and preserves dispersion in .star files',()=>{
  const samples=[
   {t:0,altitude:0,velocity:0,acceleration:40,downrange:0},
   {t:5,altitude:450,velocity:80,acceleration:-9.8,downrange:60},
   {t:25,altitude:0,velocity:8,acceleration:0,downrange:180},
  ];
  const zone=singleFlightLandingZone(samples,{windSpeed:3,windDirection:270,railAngle:5,railHeading:90},'sig-1');
  expect(zone).not.toBeNull();
  expect(zone!.ellipse.nominalEast).toBeCloseTo(180,3);
  expect(zone!.ellipse.nominalNorth).toBeCloseTo(0,3);
  const csv=landingZoneCsv(zone!);
  expect(csv).toContain('nominal_east,180.000,m');

  const project=emptyProject(),flight=initialFlight();
  project.flight=flight;
  flight.cases[0].dispersion={...defaultDispersionSettings,sampleCount:12};
  flight.cases[0].result={source:'solver',signature:'sig-1',samples,landingZone:zone!};
  const restored=parseStar(serializeStar(project)).flight!.cases[0];
  expect(restored.dispersion?.sampleCount).toBe(12);
  expect(restored.result?.landingZone?.ellipse.nominalEast).toBeCloseTo(180,3);
 });
});
