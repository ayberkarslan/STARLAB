import {type Motor,type ThrustPoint,motorStats,validateMotor} from './motors';
import type {MotorMassSample} from './motorMass';

const G0=9.80665;

export interface MotorTimeSample {
 t:number;
 thrustN:number;
 cumulativeImpulseNs:number;
 propellantMassKg:number;
 totalMassKg:number;
 massFlowRateKgS:number;
 ispS:number;
 cgFromFrontMm:number;
}

export function generateMotorTimeSeries(motor:Motor,sampleCount=60):MotorTimeSample[]{
 const stats=motorStats(motor);
 const totalImpulse=Math.max(1e-6,stats.impulse);
 const burnTime=Math.max(1e-4,stats.duration);
 const averageIsp=motor.propellantMass>0?totalImpulse/(motor.propellantMass*G0):0;
 const dryMass=Math.max(0,motor.totalMass-motor.propellantMass);

 // Collect evaluation timestamps combining curve points and uniform grid
 const tSet=new Set<number>(motor.curve.map(p=>Math.round(p.t*10000)/10000));
 for(let i=0;i<=sampleCount;i++){
  tSet.add(Math.round((burnTime*i/sampleCount)*10000)/10000);
 }
 const times=[...tSet].sort((a,b)=>a-b);

 function thrustAt(t:number):number{
  const c=motor.curve;
  if(t<=c[0].t)return c[0].thrust;
  if(t>=c[c.length-1].t)return c[c.length-1].thrust;
  for(let i=1;i<c.length;i++){
   if(t<=c[i].t){
    const dt=Math.max(1e-9,c[i].t-c[i-1].t);
    const u=(t-c[i-1].t)/dt;
    return c[i-1].thrust+u*(c[i].thrust-c[i-1].thrust);
   }
  }
  return 0;
 }

 const samples:MotorTimeSample[]=[];
 let cumImpulse=0;
 for(let i=0;i<times.length;i++){
  const t=times[i];
  const F=thrustAt(t);
  if(i>0){
   const dt=t-times[i-1];
   cumImpulse+=0.5*(samples[i-1].thrustN+F)*dt;
  }
  const burnedFrac=Math.min(1,Math.max(0,cumImpulse/totalImpulse));
  const propRem=motor.propellantMass*(1-burnedFrac);
  const totalMass=dryMass+propRem;
  const mdot=averageIsp>0?F/(averageIsp*G0):0;
  // Casing CG at 0.52 * L (nozzle aft slightly heavier), grain CG shifts slightly forward/aft during radial/end burn
  const casingCgMm=0.53*motor.length;
  const grainCgMm=(0.47-0.03*burnedFrac)*motor.length;
  const cgFromFrontMm=totalMass>1e-6?(dryMass*casingCgMm+propRem*grainCgMm)/totalMass:0.5*motor.length;
  samples.push({
   t,
   thrustN:F,
   cumulativeImpulseNs:cumImpulse,
   propellantMassKg:propRem,
   totalMassKg:totalMass,
   massFlowRateKgS:mdot,
   ispS:averageIsp,
   cgFromFrontMm,
  });
 }
 return samples;
}

export function motorTimeSeriesCsv(samples:MotorTimeSample[]):string{
 const header='time_s,thrust_n,cumulative_impulse_ns,propellant_mass_kg,total_mass_kg,mass_flow_rate_kg_s,isp_s,cg_from_front_mm';
 const lines=samples.map(s=>[
  s.t.toFixed(4),
  s.thrustN.toFixed(2),
  s.cumulativeImpulseNs.toFixed(2),
  s.propellantMassKg.toFixed(4),
  s.totalMassKg.toFixed(4),
  s.massFlowRateKgS.toFixed(4),
  s.ispS.toFixed(2),
  s.cgFromFrontMm.toFixed(1),
 ].join(','));
 return [header,...lines].join('\n');
}

export type MotorPropulsionType='solid'|'liquid'|'hybrid';
export type PropellantPresetId='apcp_solid'|'n2o_htpb_hybrid'|'lox_paraffin_hybrid'|'n2o_etoh_liquid'|'lox_rp1_liquid';
export type ThrustProfileShape='neutral'|'regressive'|'progressive'|'trapezoid';

export interface PropellantPreset {
 id:PropellantPresetId;
 label:string;
 type:MotorPropulsionType;
 defaultIspS:number;
 cStarMs:number;
 defaultOfRatio:number;
 oxDensityKgM3:number;
 fuelDensityKgM3:number;
 chamberTempK:number;
}

export const PROPELLANT_PRESETS:Record<PropellantPresetId,PropellantPreset>={
 apcp_solid:{
  id:'apcp_solid',
  label:'Katı · APCP (HTPB + NH4ClO4 + Al)',
  type:'solid',
  defaultIspS:225,
  cStarMs:1520,
  defaultOfRatio:4.0,
  oxDensityKgM3:1950,
  fuelDensityKgM3:1720,
  chamberTempK:3200,
 },
 n2o_htpb_hybrid:{
  id:'n2o_htpb_hybrid',
  label:'Hibrit · N₂O / HTPB',
  type:'hybrid',
  defaultIspS:218,
  cStarMs:1490,
  defaultOfRatio:6.5,
  oxDensityKgM3:780,
  fuelDensityKgM3:920,
  chamberTempK:3150,
 },
 lox_paraffin_hybrid:{
  id:'lox_paraffin_hybrid',
  label:'Hibrit · LOX / Parafin',
  type:'hybrid',
  defaultIspS:255,
  cStarMs:1680,
  defaultOfRatio:2.5,
  oxDensityKgM3:1141,
  fuelDensityKgM3:900,
  chamberTempK:3350,
 },
 n2o_etoh_liquid:{
  id:'n2o_etoh_liquid',
  label:'Sıvı · N₂O / Etanol',
  type:'liquid',
  defaultIspS:240,
  cStarMs:1560,
  defaultOfRatio:4.5,
  oxDensityKgM3:780,
  fuelDensityKgM3:789,
  chamberTempK:3080,
 },
 lox_rp1_liquid:{
  id:'lox_rp1_liquid',
  label:'Sıvı · LOX / RP-1',
  type:'liquid',
  defaultIspS:275,
  cStarMs:1740,
  defaultOfRatio:2.3,
  oxDensityKgM3:1141,
  fuelDensityKgM3:810,
  chamberTempK:3450,
 },
};

export interface PreliminaryMotorDesignInputs {
 name:string;
 manufacturer:string;
 presetId:PropellantPresetId;
 targetImpulseNs:number; // 8000 .. 100000 N*s per ÖTR Mİ-009
 burnTimeS:number;
 ispS:number;
 ofRatio:number;
 outerDiameterMm:number;
 chamberPressureBar:number;
 tankPressureBar:number;
 allowableStressMpa:number; // e.g. Al 6061-T6 = 276 MPa
 safetyFactor:number; // FS >= 1.5 per ÖTR s.38
 casingDensityKgM3:number; // 2700 kg/m3 for Al 6061-T6
 expansionRatio:number; // A_e / A_t
 profileShape:ThrustProfileShape;
}

export interface PreliminaryMotorDesignResult {
 averageThrustN:number;
 peakThrustN:number;
 totalPropellantMassKg:number;
 oxidizerMassKg:number;
 fuelMassKg:number;
 massFlowRateKgS:number;
 oxidizerVolumeL:number;
 fuelVolumeL:number;
 tankVolumeL:number;
 minWallThicknessMm:number;
 throatAreaMm2:number;
 throatDiameterMm:number;
 exitDiameterMm:number;
 estimatedLengthMm:number;
 dryMassKg:number;
 totalMotorMassKg:number;
 massFraction:number;
 warnings:string[];
 motor:Motor;
}

function shapeFunction(tau:number,shape:ThrustProfileShape):number{
 if(tau<=0||tau>=1)return 0;
 // Startup ramp (0..0.06) and tailoff (0.90..1.0)
 const env=tau<0.06?tau/0.06:tau>0.90?(1-tau)/0.10:1;
 if(shape==='regressive')return env*(1.22-0.44*tau);
 if(shape==='progressive')return env*(0.80+0.40*tau);
 if(shape==='trapezoid')return env;
 // neutral with slight initial spike
 return env*(1.08-0.14*tau);
}

export function computePreliminaryMotorDesign(inp:PreliminaryMotorDesignInputs):PreliminaryMotorDesignResult{
 const preset=PROPELLANT_PRESETS[inp.presetId];
 const warnings:string[]=[];
 if(inp.targetImpulseNs<8000||inp.targetImpulseNs>100000){
  warnings.push('ÖTR Mİ-009 kapsamı: Toplam impuls 8.000 – 100.000 N·s aralığı için kalibre edilmiştir.');
 }
 if(inp.safetyFactor<1.5){
  warnings.push('ÖTR Yapısal Kısıt: Emniyet katsayısı (FS) en az 1,5 olmalıdır.');
 }
 const It=Math.max(100,inp.targetImpulseNs);
 const tb=Math.max(0.5,inp.burnTimeS);
 const isp=Math.max(80,inp.ispS);
 const of=Math.max(0.2,inp.ofRatio);
 const fs=Math.max(1.0,inp.safetyFactor);
 const PcPa=Math.max(5,inp.chamberPressureBar)*1e5;
 const PtankPa=Math.max(5,inp.tankPressureBar)*1e5;
 const sigmaPa=Math.max(50,inp.allowableStressMpa)*1e6;

 const averageThrustN=It/tb;
 const totalPropellantMassKg=It/(isp*G0);
 const massFlowRateKgS=totalPropellantMassKg/tb;
 const oxidizerMassKg=preset.type==='solid'?totalPropellantMassKg*0.78:totalPropellantMassKg*(of/(1+of));
 const fuelMassKg=totalPropellantMassKg-oxidizerMassKg;

 const oxVolM3=oxidizerMassKg/preset.oxDensityKgM3;
 const fuelVolM3=fuelMassKg/preset.fuelDensityKgM3;
 const ullageFactor=preset.type==='solid'?1.18:1.12;
 const tankVolM3=(oxVolM3+fuelVolM3)*ullageFactor;

 const rOuterM=Math.max(0.02,inp.outerDiameterMm/2000);
 // Thin-walled cylinder equation from ÖTR p.38: t_cidar >= (FS * P * r) / sigma_izin
 const pDesignPa=Math.max(PcPa,PtankPa);
 const minWallThicknessM=Math.max(0.002,(fs*pDesignPa*rOuterM)/sigmaPa);
 const minWallThicknessMm=minWallThicknessM*1000;

 const throatAreaM2=(massFlowRateKgS*preset.cStarMs)/PcPa;
 const throatAreaMm2=throatAreaM2*1e6;
 const throatDiameterMm=Math.sqrt(4*throatAreaMm2/Math.PI);
 const exitDiameterMm=throatDiameterMm*Math.sqrt(Math.max(1.5,inp.expansionRatio));

 const rInnerM=Math.max(0.01,rOuterM-minWallThicknessM);
 const crossSectionM2=Math.PI*rInnerM*rInnerM;
 const activeLengthM=tankVolM3/crossSectionM2;
 const hardwareOverheadM=preset.type==='solid'?0.18:preset.type==='hybrid'?0.38:0.48;
 const estimatedLengthMm=Math.round((activeLengthM+hardwareOverheadM)*1000);

 // Dry mass: cylindrical shell + bulkheads + nozzle + valves/injector
 const shellVolM3=Math.PI*(rOuterM*rOuterM-rInnerM*rInnerM)*(activeLengthM+hardwareOverheadM*0.6);
 const bulkheadMassKg=2*Math.PI*rOuterM*rOuterM*(minWallThicknessM*1.8)*inp.casingDensityKgM3;
 const plumbingFactor=preset.type==='solid'?1.15:preset.type==='hybrid'?1.35:1.50;
 const dryMassKg=Math.max(0.5,(shellVolM3*inp.casingDensityKgM3+bulkheadMassKg)*plumbingFactor);
 const totalMotorMassKg=dryMassKg+totalPropellantMassKg;
 const massFraction=totalPropellantMassKg/totalMotorMassKg;

 // Build normalized thrust curve S(t/tb)
 const steps=40;
 const raw:ThrustPoint[]=[];
 for(let i=0;i<=steps;i++){
  const tau=i/steps;
  const s=i===0||i===steps?0:shapeFunction(tau,inp.profileShape);
  raw.push({t:Math.round(tau*tb*1000)/1000,thrust:s});
 }
 let unitImpulse=0;
 for(let i=1;i<raw.length;i++){
  unitImpulse+=0.5*(raw[i-1].thrust+raw[i].thrust)*(raw[i].t-raw[i-1].t);
 }
 const scale=It/Math.max(1e-6,unitImpulse);
 const curve:ThrustPoint[]=raw.map((p,i)=>({
  t:p.t,
  thrust:i===0||i===raw.length-1?0:Math.round(p.thrust*scale*100)/100,
 }));
 const peakThrustN=Math.max(...curve.map(p=>p.thrust));

 // Build massProfile for B3/STARLAB
 const roundedPropMass=Math.round(totalPropellantMassKg*1000)/1000;
 const roundedDryMass=Math.round(dryMassKg*1000)/1000;
 const roundedTotalMass=Math.round((roundedDryMass+roundedPropMass)*1000)/1000;
 const lengthM=estimatedLengthMm/1000;
 const massProfile:MotorMassSample[]=[];
 let cumImp=0;
 for(let i=0;i<curve.length;i++){
  if(i>0){
   cumImp+=0.5*(curve[i-1].thrust+curve[i].thrust)*(curve[i].t-curve[i-1].t);
  }
  const frac=i===0?0:i===curve.length-1?1:Math.min(1,cumImp/It);
  const propRem=i===0?roundedPropMass:Math.min(roundedPropMass,Math.max(0,Math.round(roundedPropMass*(1-frac)*10000)/10000));
  const mTot=i===0?roundedTotalMass:Math.min(roundedTotalMass,Math.round((roundedDryMass+propRem)*10000)/10000);
  const cgMm=(roundedDryMass*(0.54*estimatedLengthMm)+propRem*(0.46*estimatedLengthMm))/mTot;
  const ixx=0.5*mTot*rOuterM*rOuterM;
  const iyy=(1/12)*mTot*(3*rOuterM*rOuterM+lengthM*lengthM);
  massProfile.push({
   t:curve[i].t,
   totalMass:mTot,
   propellantMass:propRem,
   centerOfMass:Math.round(cgMm*10)/10,
   inertiaXX:Math.round(ixx*10000)/10000,
   inertiaYY:Math.round(iyy*10000)/10000,
   inertiaZZ:Math.round(iyy*10000)/10000,
  });
 }

 const motor=validateMotor({
  id:`prelim-${crypto.randomUUID().slice(0,8)}`,
  name:inp.name.trim()||'Ön Tasarım Motoru',
  manufacturer:inp.manufacturer.trim()||'STARLAB Ön Tasarım',
  diameter:Math.round(inp.outerDiameterMm),
  length:Math.max(100,estimatedLengthMm),
  totalMass:roundedTotalMass,
  propellantMass:roundedPropMass,
  source:'custom',
  curve,
  massProfile,
 });

 return {
  averageThrustN,
  peakThrustN,
  totalPropellantMassKg,
  oxidizerMassKg,
  fuelMassKg,
  massFlowRateKgS,
  oxidizerVolumeL:oxVolM3*1000,
  fuelVolumeL:fuelVolM3*1000,
  tankVolumeL:tankVolM3*1000,
  minWallThicknessMm,
  throatAreaMm2,
  throatDiameterMm,
  exitDiameterMm,
  estimatedLengthMm,
  dryMassKg,
  totalMotorMassKg,
  massFraction,
  warnings,
  motor,
 };
}

export interface CatalogMatchScore {
 motor:Motor;
 errorScore:number;
 similarityPct:number;
 impulseNs:number;
 burnTimeS:number;
}

/**
 * ÖTR s.41 Katalog Eşleştirme Hata Skoru:
 * E = w_I * |I_t - I_ref|/I_ref + w_t * |t_b - t_ref|/t_ref + w_m * |m - m_ref|/m_ref
 */
export function rankCatalogMotorsForPreliminaryDesign(
 target:{impulseNs:number;burnTimeS:number;totalMassKg:number},
 candidates:Motor[],
 weights={wI:0.5,wt:0.3,wm:0.2}
):CatalogMatchScore[]{
 return candidates.map(m=>{
  const s=motorStats(m);
  const eI=Math.abs(s.impulse-target.impulseNs)/Math.max(1,target.impulseNs);
  const et=Math.abs(s.duration-target.burnTimeS)/Math.max(0.1,target.burnTimeS);
  const em=Math.abs(m.totalMass-target.totalMassKg)/Math.max(0.1,target.totalMassKg);
  const errorScore=weights.wI*eI+weights.wt*et+weights.wm*em;
  const similarityPct=100*Math.exp(-errorScore*errorScore/(2*0.35*0.35));
  return {motor:m,errorScore,similarityPct,impulseNs:s.impulse,burnTimeS:s.duration};
 }).sort((a,b)=>a.errorScore-b.errorScore);
}
