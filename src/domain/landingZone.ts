import type {FlightSample} from './flight';

export interface LandingDispersionSettings {
 sampleCount:number;
 seed:number;
 windSpeedSigma:number;
 windDirectionSigma:number;
 railAngleSigma:number;
 railHeadingSigma:number;
 thrustScaleSigma:number;
 dragScaleSigma:number;
 massScaleSigma?:number;
 cgOffsetSigmaMm?:number;
 parachuteCdSigma?:number;
 payloadEnabled?:boolean;
 payloadMassKg?:number;
 payloadChuteAreaM2?:number;
 payloadChuteCd?:number;
 boosterChuteAreaM2?:number;
 boosterChuteCd?:number;
}

export interface LandingTrajectoryPoint {
 t:number;
 east:number;
 north:number;
 altitude:number;
}

export interface LandingPoint {
 index:number;
 nominal?:boolean;
 east:number;
 north:number;
 apogee:number;
 flightTime:number;
 impactSpeed:number;
 windSpeed:number;
 windDirection:number;
 railAngle:number;
 railHeading:number;
 thrustScale:number;
 dragScale:number;
 trajectory:LandingTrajectoryPoint[];
}

export interface LandingZoneEllipse {
 centerEast:number;
 centerNorth:number;
 nominalEast:number;
 nominalNorth:number;
 semiMajor1Sigma:number;
 semiMinor1Sigma:number;
 semiMajor2Sigma:number;
 semiMinor2Sigma:number;
 semiMajor3Sigma:number;
 semiMinor3Sigma:number;
 /** Angle in degrees counterclockwise from +East toward +North. */
 angleDeg:number;
 /** Bearing of major axis in degrees clockwise from North (0..360). */
 azimuthDeg:number;
 cep50Radius:number;
 area3SigmaM2:number;
 meanRangeM:number;
 meanBearingDeg:number;
 maxRangeM:number;
}

export interface LandingBodyZone {
 id:'sustainer'|'payload'|'booster';
 name:string;
 ellipse:LandingZoneEllipse;
 points:LandingPoint[];
}

export interface LandingZoneResult {
 signature:string;
 solverId?:string;
 settings:LandingDispersionSettings;
 ellipse:LandingZoneEllipse;
 points:LandingPoint[];
 bodies?:LandingBodyZone[];
}

export const defaultDispersionSettings:LandingDispersionSettings={
 sampleCount:16,
 seed:42,
 windSpeedSigma:1.5,
 windDirectionSigma:12,
 railAngleSigma:1,
 railHeadingSigma:3,
 thrustScaleSigma:2,
 dragScaleSigma:5,
 massScaleSigma:1.5,
 cgOffsetSigmaMm:8,
 parachuteCdSigma:6,
 payloadEnabled:false,
 payloadMassKg:4,
 payloadChuteAreaM2:0.95,
 payloadChuteCd:0.8,
 boosterChuteAreaM2:1.2,
 boosterChuteCd:0.8,
};

export function validateDispersionSettings(value:unknown):LandingDispersionSettings {
 if(!value||typeof value!=='object'||Array.isArray(value))throw Error('İniş bölgesi dağılım ayarları geçersiz.');
 const v=value as LandingDispersionSettings;
 if(!Number.isInteger(v.sampleCount)||v.sampleCount<4||v.sampleCount>64)throw Error('Dağılım örnek sayısı 4 ile 64 arasında tam sayı olmalı.');
 if(!Number.isInteger(v.seed)||v.seed<0||v.seed>1_000_000_000)throw Error('Rastgelelik tohumu geçersiz.');
 const checks:[keyof LandingDispersionSettings,number][]=[
  ['windSpeedSigma',50],['windDirectionSigma',180],['railAngleSigma',30],
  ['railHeadingSigma',180],['thrustScaleSigma',50],['dragScaleSigma',100],
 ];
 for(const [key,max] of checks){
  const n=v[key];
  if(typeof n!=='number'||!Number.isFinite(n)||n<0||n>max)throw Error('İniş bölgesi belirsizlik girdisi geçersiz.');
 }
 const optChecks:[keyof LandingDispersionSettings,number,number][]=[
  ['massScaleSigma',0,50],['cgOffsetSigmaMm',0,500],['parachuteCdSigma',0,80],
  ['payloadMassKg',0.05,500],['payloadChuteAreaM2',0.01,100],['payloadChuteCd',0.1,3],
  ['boosterChuteAreaM2',0,100],['boosterChuteCd',0.1,3],
 ];
 for(const [key,min,max] of optChecks){
  const n=v[key];
  if(n!==undefined&&(typeof n!=='number'||!Number.isFinite(n)||n<min||n>max))throw Error('İniş bölgesi ek tolerans/kurtarma girdisi geçersiz.');
 }
 if(v.payloadEnabled!==undefined&&typeof v.payloadEnabled!=='boolean')throw Error('Görev yükü kurtarma seçimi geçersiz.');
 return v;
}

export function validateLandingZoneResult(value:unknown):LandingZoneResult {
 if(!value||typeof value!=='object'||Array.isArray(value))throw Error('İniş bölgesi sonucu geçersiz.');
 const v=value as LandingZoneResult;
 if(typeof v.signature!=='string'||!v.ellipse||!Array.isArray(v.points)||v.points.length<1||v.points.length>65)throw Error('İniş bölgesi sonuç kaydı geçersiz.');
 validateDispersionSettings(v.settings);
 const e=v.ellipse;
 if(![e.centerEast,e.centerNorth,e.nominalEast,e.nominalNorth,e.semiMajor1Sigma,e.semiMinor1Sigma,e.semiMajor2Sigma,e.semiMinor2Sigma,e.semiMajor3Sigma,e.semiMinor3Sigma,e.angleDeg,e.azimuthDeg,e.cep50Radius,e.area3SigmaM2,e.meanRangeM,e.meanBearingDeg,e.maxRangeM].every(Number.isFinite))throw Error('İniş bölgesi elips değerleri geçersiz.');
 for(const p of v.points){
  if(!p||![p.index,p.east,p.north,p.apogee,p.flightTime,p.impactSpeed,p.windSpeed,p.windDirection,p.railAngle,p.railHeading,p.thrustScale,p.dragScale].every(Number.isFinite)||!Array.isArray(p.trajectory)||p.trajectory.length>500)throw Error('İniş noktası kaydı geçersiz.');
  for(const t of p.trajectory)if(!t||![t.t,t.east,t.north,t.altitude].every(Number.isFinite))throw Error('İniş yörünge noktası geçersiz.');
 }
 if(v.bodies!==undefined){
  if(!Array.isArray(v.bodies)||v.bodies.length>4)throw Error('Kurtarılan bileşen bölgeleri geçersiz.');
  for(const b of v.bodies){
   if(!b||!['sustainer','payload','booster'].includes(b.id)||typeof b.name!=='string'||!b.ellipse||!Array.isArray(b.points)||b.points.length<1)throw Error('Kurtarılan bileşen bölgesi geçersiz.');
  }
 }
 return v;
}

export function mulberry32(seed:number){
 let state=seed>>>0;
 return ()=>{
  state=(state+0x6D2B79F5)>>>0;
  let t=Math.imul(state^(state>>>15),1|state);
  t=(t+Math.imul(t^(t>>>7),61|t))^t;
  return ((t^(t>>>14))>>>0)/4294967296;
 };
}

export function gaussianPair(rng:()=>number):[number,number]{
 const u1=Math.max(1e-12,rng()),u2=rng();
 const radius=Math.sqrt(-2*Math.log(u1)),theta=2*Math.PI*u2;
 return [radius*Math.cos(theta),radius*Math.sin(theta)];
}

export function bearingFromEnu(east:number,north:number){
 const deg=Math.atan2(east,north)*180/Math.PI;
 return ((deg%360)+360)%360;
}

export function computeLandingEllipse(points:{east:number;north:number;nominal?:boolean}[]):LandingZoneEllipse {
 if(!points.length)throw Error('İniş elipsi için en az bir nokta gerekli.');
 const nominal=points.find(p=>p.nominal)??points[0];
 const n=points.length;
 const centerEast=points.reduce((sum,p)=>sum+p.east,0)/n;
 const centerNorth=points.reduce((sum,p)=>sum+p.north,0)/n;
 let cEE=0,cNN=0,cEN=0;
 if(n>1){
  for(const p of points){
   const dE=p.east-centerEast,dN=p.north-centerNorth;
   cEE+=dE*dE;cNN+=dN*dN;cEN+=dE*dN;
  }
  cEE/=n-1;cNN/=n-1;cEN/=n-1;
 }
 const mean=0.5*(cEE+cNN),diff=0.5*(cEE-cNN);
 const disc=Math.hypot(diff,cEN);
 const lambda1=Math.max(0,mean+disc),lambda2=Math.max(0,mean-disc);
 const semiMajor1Sigma=Math.sqrt(lambda1),semiMinor1Sigma=Math.sqrt(lambda2);
 const angleRad=0.5*Math.atan2(2*cEN,cEE-cNN);
 const angleDeg=angleRad*180/Math.PI;
 const azimuthDeg=((90-angleDeg)%360+360)%360;
 const cep50Radius=0.5887*(semiMajor1Sigma+semiMinor1Sigma);
 const area3SigmaM2=Math.PI*(3*semiMajor1Sigma)*(3*semiMinor1Sigma);
 const meanRangeM=Math.hypot(centerEast,centerNorth);
 const meanBearingDeg=bearingFromEnu(centerEast,centerNorth);
 const maxRangeM=Math.max(...points.map(p=>Math.hypot(p.east,p.north)));
 return {
  centerEast,centerNorth,
  nominalEast:nominal.east,nominalNorth:nominal.north,
  semiMajor1Sigma,semiMinor1Sigma,
  semiMajor2Sigma:2*semiMajor1Sigma,semiMinor2Sigma:2*semiMinor1Sigma,
  semiMajor3Sigma:3*semiMajor1Sigma,semiMinor3Sigma:3*semiMinor1Sigma,
  angleDeg,azimuthDeg,cep50Radius,area3SigmaM2,
  meanRangeM,meanBearingDeg,maxRangeM,
 };
}

export function decimateTrajectory(samples:{t:number;east:number;north:number;altitude:number}[],maxPoints=140):LandingTrajectoryPoint[]{
 if(samples.length<=maxPoints)return samples.map(s=>({t:Number(s.t.toFixed(3)),east:Number(s.east.toFixed(2)),north:Number(s.north.toFixed(2)),altitude:Number(s.altitude.toFixed(2))}));
 const stride=Math.max(1,Math.ceil(samples.length/maxPoints));
 let apexIndex=0;for(let i=1;i<samples.length;i++)if(samples[i].altitude>samples[apexIndex].altitude)apexIndex=i;
 return samples.filter((_,i)=>i===0||i===apexIndex||i===samples.length-1||i%stride===0).map(s=>({t:Number(s.t.toFixed(3)),east:Number(s.east.toFixed(2)),north:Number(s.north.toFixed(2)),altitude:Number(s.altitude.toFixed(2))}));
}

export function singleFlightLandingZone(samples:FlightSample[],environment:{windSpeed:number;windDirection:number;railAngle:number;railHeading:number},signature=''):LandingZoneResult|null {
 if(samples.length<2)return null;
 const headingRad=environment.railHeading*Math.PI/180;
 const raw=samples.map(s=>{
  const downrange=s.downrange??0;
  const east=s.east!==undefined?s.east:downrange*Math.sin(headingRad);
  const north=s.north!==undefined?s.north:downrange*Math.cos(headingRad);
  return {t:s.t,east,north,altitude:Math.max(0,s.altitude)};
 });
 const last=raw.at(-1)!,lastSample=samples.at(-1)!;
 const point:LandingPoint={
  index:0,nominal:true,
  east:last.east,north:last.north,
  apogee:Math.max(...samples.map(s=>s.altitude)),
  flightTime:lastSample.t,
  impactSpeed:Math.abs(lastSample.velocity),
  windSpeed:environment.windSpeed,
  windDirection:environment.windDirection,
  railAngle:environment.railAngle,
  railHeading:environment.railHeading,
  thrustScale:1,dragScale:1,
  trajectory:decimateTrajectory(raw),
 };
 return {
  signature,
  settings:defaultDispersionSettings,
  ellipse:computeLandingEllipse([point]),
  points:[point],
 };
}

export function monteCarloAltitudeEnvelope(result?:LandingZoneResult){
 if(!result||result.points.length<=1)return null;
 const apogees=result.points.map(p=>p.apogee);
 const minApogee=Math.min(...apogees),maxApogee=Math.max(...apogees);
 const meanApogee=apogees.reduce((a,b)=>a+b,0)/apogees.length;
 const maxT=Math.max(...result.points.map(p=>p.flightTime));
 if(!(maxT>0))return null;
 const steps=60;
 const minSeries:[number,number][]=[],maxSeries:[number,number][]=[];
 for(let i=0;i<=steps;i++){
  const t=(maxT*i)/steps;
  let lo=Infinity,hi=-Infinity;
  for(const p of result.points){
   const tr=p.trajectory;
   if(!tr.length)continue;
   let alt=0;
   if(t<=tr[0].t)alt=tr[0].altitude;
   else if(t>=tr[tr.length-1].t)alt=0;
   else{
    for(let j=1;j<tr.length;j++){
     if(t<=tr[j].t){
      const span=Math.max(1e-6,tr[j].t-tr[j-1].t);
      alt=tr[j-1].altitude+(tr[j].altitude-tr[j-1].altitude)*((t-tr[j-1].t)/span);
      break;
     }
    }
   }
   if(alt<lo)lo=alt;
   if(alt>hi)hi=alt;
  }
  if(Number.isFinite(lo)&&Number.isFinite(hi)){
   minSeries.push([Number(t.toFixed(2)),Number(lo.toFixed(1))]);
   maxSeries.push([Number(t.toFixed(2)),Number(hi.toFixed(1))]);
  }
 }
 return {minSeries,maxSeries,minApogee,maxApogee,meanApogee};
}

export function landingZoneCsv(result:LandingZoneResult){
 const e=result.ellipse;
 const rows=[
  'metric,value,unit',
  `center_east,${e.centerEast.toFixed(3)},m`,
  `center_north,${e.centerNorth.toFixed(3)},m`,
  `nominal_east,${e.nominalEast.toFixed(3)},m`,
  `nominal_north,${e.nominalNorth.toFixed(3)},m`,
  `mean_range,${e.meanRangeM.toFixed(3)},m`,
  `mean_bearing,${e.meanBearingDeg.toFixed(2)},deg`,
  `max_range,${e.maxRangeM.toFixed(3)},m`,
  `cep50_radius,${e.cep50Radius.toFixed(3)},m`,
  `semi_major_1sigma,${e.semiMajor1Sigma.toFixed(3)},m`,
  `semi_minor_1sigma,${e.semiMinor1Sigma.toFixed(3)},m`,
  `semi_major_2sigma,${e.semiMajor2Sigma.toFixed(3)},m`,
  `semi_minor_2sigma,${e.semiMinor2Sigma.toFixed(3)},m`,
  `semi_major_3sigma,${e.semiMajor3Sigma.toFixed(3)},m`,
  `semi_minor_3sigma,${e.semiMinor3Sigma.toFixed(3)},m`,
  `ellipse_azimuth,${e.azimuthDeg.toFixed(2)},deg`,
  `area_3sigma,${e.area3SigmaM2.toFixed(2)},m2`,
  ...(result.bodies?.flatMap(b=>[
   `${b.id}_center_east,${b.ellipse.centerEast.toFixed(3)},m`,
   `${b.id}_center_north,${b.ellipse.centerNorth.toFixed(3)},m`,
   `${b.id}_cep50_radius,${b.ellipse.cep50Radius.toFixed(3)},m`,
   `${b.id}_semi_major_3sigma,${b.ellipse.semiMajor3Sigma.toFixed(3)},m`,
   `${b.id}_semi_minor_3sigma,${b.ellipse.semiMinor3Sigma.toFixed(3)},m`,
  ])??[]),
  '',
  'body,run,nominal,east_m,north_m,range_m,bearing_deg,apogee_m,flight_time_s,impact_speed_m_s,wind_speed_m_s,wind_direction_deg,rail_angle_deg,rail_heading_deg,thrust_scale,drag_scale',
  ...(result.bodies?.length
   ?result.bodies.flatMap(b=>b.points.map(p=>`${b.id},${p.index},${p.nominal?1:0},${p.east.toFixed(3)},${p.north.toFixed(3)},${Math.hypot(p.east,p.north).toFixed(3)},${bearingFromEnu(p.east,p.north).toFixed(2)},${p.apogee.toFixed(2)},${p.flightTime.toFixed(2)},${p.impactSpeed.toFixed(2)},${p.windSpeed.toFixed(2)},${p.windDirection.toFixed(2)},${p.railAngle.toFixed(2)},${p.railHeading.toFixed(2)},${p.thrustScale.toFixed(4)},${p.dragScale.toFixed(4)}`))
   :result.points.map(p=>`sustainer,${p.index},${p.nominal?1:0},${p.east.toFixed(3)},${p.north.toFixed(3)},${Math.hypot(p.east,p.north).toFixed(3)},${bearingFromEnu(p.east,p.north).toFixed(2)},${p.apogee.toFixed(2)},${p.flightTime.toFixed(2)},${p.impactSpeed.toFixed(2)},${p.windSpeed.toFixed(2)},${p.windDirection.toFixed(2)},${p.railAngle.toFixed(2)},${p.railHeading.toFixed(2)},${p.thrustScale.toFixed(4)},${p.dragScale.toFixed(4)}`)),
 ];
 return rows.join('\n');
}
