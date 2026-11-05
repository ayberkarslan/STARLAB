import type {AeroCoefficientRow} from './aerodynamics';
import {resolveParts,boundsOf} from './geometry';
import type {Project} from './project';

export interface MachAeroPoint {
 mach:number;
 reynolds:number;
 cdOff:number;
 cdOn:number;
 cnAlphaPerRad:number;
 cn:number;
 cm:number;
 cpMm:number;
 marginCal:number;
 regime:'Ses altı'|'Transonik'|'Süpersonik';
}

export interface FinFlutterDiagnostic {
 partId:string;
 name:string;
 rootChordMm:number;
 tipChordMm:number;
 spanMm:number;
 thicknessMm:number;
 sweepDeg:number;
 shearModulusGpa:number;
 flutterSpeedMs:number;
 flutterMach:number;
 normalShockCritMach:number;
 safetyRatio?:number;
 status:'safe'|'warning'|'critical';
}

export const defaultMachSweepGrid=[0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.95,1.0,1.05,1.1,1.2,1.4,1.6,1.8,2.0] as const;

function compressibilityFactor(mach:number):number{
 if(mach<=0.8)return 1/Math.sqrt(Math.max(0.08,1-mach*mach));
 if(mach>=1.2)return 1/Math.sqrt(Math.max(0.08,mach*mach-1));
 const sub=1/Math.sqrt(1-0.8*0.8);
 const sup=1/Math.sqrt(1.2*1.2-1);
 const u=(mach-0.8)/0.4;
 const h00=2*u*u*u-3*u*u+1,h01=-2*u*u*u+3*u*u;
 const peakBump=0.22*Math.sin(Math.PI*u);
 return h00*sub+h01*sup+peakBump;
}

function estimateStaticAeroGeometry(project:Project){
 const resolved=resolveParts(project);
 const bounds=boundsOf(resolved);
 const totalLengthMm=Math.max(100,bounds.max[0]-bounds.min[0]);
 const refDiameterMm=project.aerodynamics?.referenceDiameter
  ??Math.max(20,...project.parts.filter(p=>['body','nose','transition'].includes(p.kind)).map(p=>p.diameter));
 const refRadiusM=refDiameterMm/2000;
 const refAreaM2=project.aerodynamics?.referenceArea??(Math.PI*refRadiusM*refRadiusM);
 let sumCna=0,sumMomentMm=0,wettedAreaM2=0;

 for(const {part,position} of resolved){
  const xMm=position[0];
  if(part.kind==='nose'){
   const cna=part.inputs?.normalForceSlope??2.0;
   const profile=part.geometry?.noseProfile??'conical';
   const cpFrac=profile==='conical'?0.666:profile==='tangentOgive'||profile==='secantOgive'?0.466:profile==='elliptical'?0.333:0.5;
   const cpLocalMm=part.inputs?.centerOfPressure??(part.length*cpFrac);
   sumCna+=cna;
   sumMomentMm+=cna*(xMm+cpLocalMm);
   wettedAreaM2+=0.68*Math.PI*(part.diameter/1000)*(part.length/1000);
  }else if(part.kind==='body'){
   wettedAreaM2+=Math.PI*(part.diameter/1000)*(part.length/1000);
  }else if(part.kind==='transition'){
   const d1=Math.max(1,part.diameter),d2=Math.max(1,part.geometry?.aftDiameter??part.diameter);
   const rRatio1=d1/refDiameterMm,rRatio2=d2/refDiameterMm;
   const cna=part.inputs?.normalForceSlope??Math.abs(2*(rRatio2*rRatio2-rRatio1*rRatio1));
   const cpLocalMm=part.inputs?.centerOfPressure??(part.length*0.5);
   sumCna+=cna;
   sumMomentMm+=cna*(xMm+cpLocalMm);
   wettedAreaM2+=Math.PI*((d1+d2)/2000)*(part.length/1000);
  }else if(part.kind==='fins'){
   const parent=project.parts.find(p=>p.id===part.parentId);
   const bodyDiaMm=parent?.diameter??refDiameterMm;
   const spanMm=Math.max(5,(part.diameter-bodyDiaMm)/2);
   const cr=Math.max(10,part.length),ct=Math.max(5,part.geometry?.tipChord??cr*0.5);
   const sweep=part.geometry?.sweep??(cr-ct)*0.5;
   const nFins=part.geometry?.finCount??4;
   const midChord=Math.hypot(spanMm,sweep+0.5*(ct-cr));
   const kfb=1+(bodyDiaMm/2)/(spanMm+bodyDiaMm/2);
   const denom=1+Math.sqrt(1+Math.pow(2*midChord/(cr+ct),2));
   const rawCna=(4*nFins*Math.pow(spanMm/refDiameterMm,2))/denom;
   const cna=part.inputs?.normalForceSlope??(kfb*rawCna);
   const xF=sweep*(cr+2*ct)/(3*(cr+ct))+(cr*cr+cr*ct+ct*ct)/(6*(cr+ct));
   const cpLocalMm=part.inputs?.centerOfPressure??xF;
   sumCna+=cna;
   sumMomentMm+=cna*(xMm+cpLocalMm);
   const finAreaM2=0.5*((cr+ct)/1000)*(spanMm/1000);
   wettedAreaM2+=2*nFins*finAreaM2;
  }
 }

 const baseCna=project.aerodynamics?.normalForceSlope??Math.max(4,sumCna);
 const baseCpMm=project.aerodynamics?.centerOfPressure??(sumCna>0?sumMomentMm/sumCna:totalLengthMm*0.68);
 const roughnessM=(project.aerodynamics?.surfaceRoughness??20)*1e-6;
 return {totalLengthMm,refDiameterMm,refAreaM2,wettedAreaM2:Math.max(refAreaM2*4,wettedAreaM2),baseCna,baseCpMm,roughnessM};
}

export function generateMachAeroTable(project:Project,cgMm?:number,aoaDeg=4,machGrid:readonly number[]=defaultMachSweepGrid):MachAeroPoint[]{
 const geom=estimateStaticAeroGeometry(project);
 const effectiveCgMm=cgMm??(geom.totalLengthMm*0.56);
 const speedOfSound=340.29,kinematicVisc=1.46e-5;
 const lengthM=geom.totalLengthMm/1000;
 const fineness=Math.max(3,geom.totalLengthMm/geom.refDiameterMm);
 const aoaRad=(aoaDeg*Math.PI)/180;
 const sinA=Math.sin(aoaRad),cosA=Math.cos(aoaRad);

 return machGrid.map(mach=>{
  const velocity=mach*speedOfSound;
  const reynolds=Math.max(1e4,(velocity*lengthM)/kinematicVisc);
  const reCrit=51*Math.pow(Math.max(1e-7,geom.roughnessM)/lengthM,-1.039);
  const reEff=Math.min(reynolds,Math.max(1e5,reCrit));
  const cfIncomp=0.455/Math.pow(Math.log10(reEff),2.58);
  const cf=cfIncomp/(1+0.144*mach*mach);
  const bodyFormFactor=1+60/Math.pow(fineness,3)+0.0025*fineness;
  const cdFriction=cf*bodyFormFactor*(geom.wettedAreaM2/geom.refAreaM2);
  const cdBaseOff=mach<1?0.12+0.13*mach*mach:0.25/Math.max(1,mach);
  const cdBaseOn=cdBaseOff*0.58; // Jet plume base-drag reduction during powered flight
  const cdWave=mach<=0.85?0:mach<1.15?0.24*Math.pow((mach-0.85)/0.3,2)*(3-2*((mach-0.85)/0.3)):0.24/Math.sqrt(Math.max(0.15,mach*mach-1))*Math.pow(10/fineness,0.6);

  const comp=compressibilityFactor(mach);
  const cnAlphaPerRad=geom.baseCna*(0.75+0.25*comp);
  const cpShiftMm=mach<=0.85?0:Math.min(geom.totalLengthMm*0.09,geom.refDiameterMm*0.42*(mach-0.85));
  const cpMm=Math.min(geom.totalLengthMm*0.92,geom.baseCpMm+cpShiftMm);
  const cn=cnAlphaPerRad*sinA*cosA+1.1*sinA*sinA*Math.sign(aoaRad||1);
  const cdInduced=Math.abs(cn*sinA)*0.65;
  const cdOff= (project.aerodynamics?.dragCoefficient??(cdFriction+cdBaseOff))+cdWave+cdInduced;
  const cdOn=Math.max(0.08,cdOff-(cdBaseOff-cdBaseOn));
  const marginCal=(cpMm-effectiveCgMm)/geom.refDiameterMm;
  const cm=-cn*marginCal;
  const regime:MachAeroPoint['regime']=mach<0.85?'Ses altı':mach<=1.15?'Transonik':'Süpersonik';
  return {
   mach:Number(mach.toFixed(2)),
   reynolds:Math.round(reynolds),
   cdOff:Number(cdOff.toFixed(4)),
   cdOn:Number(cdOn.toFixed(4)),
   cnAlphaPerRad:Number(cnAlphaPerRad.toFixed(4)),
   cn:Number(cn.toFixed(4)),
   cm:Number(cm.toFixed(4)),
   cpMm:Number(cpMm.toFixed(1)),
   marginCal:Number(marginCal.toFixed(3)),
   regime,
  };
 });
}

export function machAeroTableToRasaeroGrid(project:Project,cgMm?:number):AeroCoefficientRow[]{
 const machs=[0.3,0.6,0.8,0.95,1.05,1.2,1.5,2.0];
 const reynoldsList=[1_000_000,5_000_000];
 const aoas=[0,4,8];
 const rows:AeroCoefficientRow[]=[];
 for(const Mach of machs){
  for(const Reynolds of reynoldsList){
   for(const AoA_deg of aoas){
    const pt=generateMachAeroTable(project,cgMm,AoA_deg,[Mach])[0];
    const reScale=Reynolds>2_500_000?0.96:1.0;
    rows.push({
     Mach,
     Reynolds,
     AoA_deg,
     CA_off:Number((pt.cdOff*reScale).toFixed(4)),
     CA_on:Number((pt.cdOn*reScale).toFixed(4)),
     CN:Number(pt.cn.toFixed(4)),
     CP_m:Number((pt.cpMm/1000).toFixed(4)),
     CN_alpha_per_rad:Number(pt.cnAlphaPerRad.toFixed(4)),
    });
   }
  }
 }
 return rows;
}

export function machAeroTableCsv(rows:MachAeroPoint[]):string{
 return [
  'Mach,Reynolds,CD_takatsiz,CD_takatli,CN_alpha_rad,CN,Cm,CP_mm,statik_marj_cal,rejim',
  ...rows.map(r=>`${r.mach},${r.reynolds},${r.cdOff},${r.cdOn},${r.cnAlphaPerRad},${r.cn},${r.cm},${r.cpMm},${r.marginCal},${r.regime}`),
 ].join('\n');
}

export function rasaeroTableCsv(rows:AeroCoefficientRow[]):string{
 return [
  'Mach,Reynolds,AoA_deg,CA_off,CA_on,CN,CP_m,CN_alpha_per_rad',
  ...rows.map(r=>`${r.Mach},${r.Reynolds},${r.AoA_deg},${r.CA_off},${r.CA_on},${r.CN},${r.CP_m},${r.CN_alpha_per_rad}`),
 ].join('\n');
}

export function computeFinFlutterDiagnostics(project:Project,maxFlightSpeedMs?:number):FinFlutterDiagnostic[]{
 const speedOfSound=340.29;
 const fins=project.parts.filter(p=>p.kind==='fins');
 return fins.map(part=>{
  const parent=project.parts.find(p=>p.id===part.parentId);
  const bodyDiaMm=parent?.diameter??140;
  const spanMm=Math.max(10,(part.diameter-bodyDiaMm)/2);
  const cr=Math.max(15,part.length);
  const ct=Math.max(5,part.geometry?.tipChord??cr*0.5);
  const thicknessMm=Math.max(0.8,part.inputs?.wallThickness??3);
  const sweepMm=part.geometry?.sweep??(cr-ct)*0.5;
  const sweepRad=Math.atan2(sweepMm,spanMm);
  const sweepDeg=(sweepRad*180)/Math.PI;
  const density=part.inputs?.density??1850;
  // Estimate shear modulus G (Pa) from structural material density/name
  const matName=(part.material||'').toLowerCase();
  const shearModulusGpa=
   /karbon|carbon/.test(matName)?18.0:
   /alüminyum|alumin|7075|6061/.test(matName)||density>=2600?26.0:
   /cam|fiberglas|g10|fr4|kompozit/.test(matName)||density>=1700?11.0:
   4.2;
  const G=shearModulusGpa*1e9;
  const P0=101325;
  const areaMm2=0.5*(cr+ct)*spanMm;
  const aspectRatio=(spanMm*spanMm)/Math.max(1,areaMm2);
  const taperRatio=ct/cr;
  const thicknessRatio=thicknessMm/cr;
  // NACA TN-4197 subsonic/transonic fin flutter velocity at sea level
  const term=(G/(1.337*Math.pow(aspectRatio,3)*P0*(taperRatio+1)))*2*Math.pow(thicknessRatio,3);
  const flutterMach=Math.sqrt(Math.max(0.05,term));
  const flutterSpeedMs=flutterMach*speedOfSound;
  const normalShockCritMach=1/Math.max(0.25,Math.cos(sweepRad)*0.94);
  const safetyRatio=maxFlightSpeedMs&&maxFlightSpeedMs>0?flutterSpeedMs/maxFlightSpeedMs:undefined;
  const status:FinFlutterDiagnostic['status']=
   safetyRatio!==undefined?(safetyRatio<1.15?'critical':safetyRatio<1.45?'warning':'safe'):
   flutterMach<0.85?'warning':'safe';
  return {
   partId:part.id,
   name:part.name,
   rootChordMm:Number(cr.toFixed(1)),
   tipChordMm:Number(ct.toFixed(1)),
   spanMm:Number(spanMm.toFixed(1)),
   thicknessMm:Number(thicknessMm.toFixed(2)),
   sweepDeg:Number(sweepDeg.toFixed(1)),
   shearModulusGpa:Number(shearModulusGpa.toFixed(1)),
   flutterSpeedMs:Number(flutterSpeedMs.toFixed(1)),
   flutterMach:Number(flutterMach.toFixed(2)),
   normalShockCritMach:Number(normalShockCritMach.toFixed(2)),
   ...(safetyRatio!==undefined?{safetyRatio:Number(safetyRatio.toFixed(2))}:{}),
   status,
  };
 });
}
