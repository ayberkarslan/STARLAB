import {describe,expect,it} from 'vitest';
import {computeFinFlutterDiagnostics,generateMachAeroTable} from './aeroSweep';
import {generateEnvironmentProfile,parseEnvironmentProfileJson,serializeEnvironmentProfileJson} from './environmentProfile';
import {defaultEnvironment} from './flight';
import {computePreliminaryMotorDesign,generateMotorTimeSeries,rankCatalogMotorsForPreliminaryDesign} from './motorPreliminaryDesign';
import {sampleMotors} from './motors';
import {generateVerificationReportMarkdown,openRocketBenchmarkProject,twoStagePayloadBenchmarkProject} from './verificationBenchmarks';

describe('KTR / ÖTR software modules',()=>{
 it('generates Mach 0.30-2.00 aero sweep and fin flutter diagnostics',()=>{
  const project=openRocketBenchmarkProject();
  const rows=generateMachAeroTable(project,2060,2);
  expect(rows[0].mach).toBeCloseTo(0.3,2);
  expect(rows.at(-1)!.mach).toBeCloseTo(2.0,2);
  expect(rows.every(r=>r.cdOn<=r.cdOff&&r.cpMm>0)).toBe(true);
  const flutter=computeFinFlutterDiagnostics(project,225);
  expect(flutter.length).toBe(1);
  expect(flutter[0].flutterSpeedMs).toBeGreaterThan(200);
 });

 it('generates altitude environment profile and roundtrips JSON',()=>{
  const samples=generateEnvironmentProfile({...defaultEnvironment,atmosphereModel:'standard1976',windModel:'powerLaw',windSpeed:6},3000,500);
  expect(samples.length).toBe(7);
  expect(samples[6].pressurePa).toBeLessThan(samples[0].pressurePa);
  expect(samples[6].windSpeedMs).toBeGreaterThan(samples[1].windSpeedMs);
  const json=serializeEnvironmentProfileJson(defaultEnvironment);
  const parsed=parseEnvironmentProfileJson(json,defaultEnvironment);
  expect(parsed.pressure).toBe(101325);
 });

 it('computes motor time series and solid/liquid/hybrid preliminary design',()=>{
  const series=generateMotorTimeSeries(sampleMotors[2],20);
  expect(series.length).toBeGreaterThan(15);
  expect(series[0].ispS).toBeGreaterThan(100);
  expect(series.at(-1)!.propellantMassKg).toBeCloseTo(0,3);

  const prelim=computePreliminaryMotorDesign({
   name:'STAR-H25K',
   manufacturer:'STARLAB',
   presetId:'n2o_htpb_hybrid',
   targetImpulseNs:25000,
   burnTimeS:8,
   ispS:218,
   ofRatio:6.5,
   outerDiameterMm:150,
   chamberPressureBar:35,
   tankPressureBar:55,
   allowableStressMpa:276,
   safetyFactor:1.8,
   casingDensityKgM3:2700,
   expansionRatio:6,
   profileShape:'regressive',
  });
  expect(prelim.minWallThicknessMm).toBeGreaterThan(2);
  expect(prelim.motor.totalMass).toBeGreaterThan(prelim.motor.propellantMass);
  const ranked=rankCatalogMotorsForPreliminaryDesign({impulseNs:5000,burnTimeS:2.4,totalMassKg:6.5},sampleMotors);
  expect(ranked[0].motor.id).toBe('sample-98');
 });

 it('builds 1-stage OpenRocket and 2-stage KTR benchmark projects and report',()=>{
  const p1=openRocketBenchmarkProject();
  const p2=twoStagePayloadBenchmarkProject();
  expect(p1.parts.filter(p=>p.kind==='stage').length).toBe(1);
  expect(p2.parts.filter(p=>p.kind==='stage').length).toBe(2);
  const md=generateVerificationReportMarkdown(p2,p2.flight!);
  expect(md).toContain('STARLAB Doğrulama ve KTR Uyumluluk Raporu');
 });
});
