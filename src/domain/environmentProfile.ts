import {validateEnvironment,type Environment} from './flight';
import type {AtmospherePoint,WindPoint} from './environmentInputs';

export interface EnvironmentProfileSample {
 altitudeAglM:number;
 altitudeAslM:number;
 temperatureK:number;
 temperatureC:number;
 pressurePa:number;
 densityKgM3:number;
 speedOfSoundMs:number;
 dynamicViscosityPas:number;
 windSpeedMs:number;
 windDirectionDeg:number;
 windEastMs:number;
 windNorthMs:number;
}

const R_AIR=287.05287;
const GAMMA_AIR=1.4;
const G0=9.80665;
const R_EARTH=6356766;

function sutherlandViscosity(T:number):number{
 const T0=288.15,mu0=1.7894e-5,S=110.4;
 const Tc=Math.max(100,T);
 return mu0*Math.pow(Tc/T0,1.5)*(T0+S)/(Tc+S);
}

function us1976AtAsl(aslM:number,T0_override?:number,P0_override?:number):{T:number;P:number;rho:number;a:number;mu:number}{
 const h=R_EARTH*aslM/(R_EARTH+Math.max(-600,aslM));
 const T0=T0_override??288.15;
 const P0=P0_override??101325;
 const L=-0.0065;
 let T:number,P:number;
 if(h<=11000){
  T=Math.max(180,T0+L*h);
  P=P0*Math.pow(T/T0,-G0/(L*R_AIR));
 }else if(h<=20000){
  const T11=Math.max(180,T0+L*11000);
  const P11=P0*Math.pow(T11/T0,-G0/(L*R_AIR));
  T=T11;
  P=P11*Math.exp(-G0*(h-11000)/(R_AIR*T11));
 }else{
  const T11=Math.max(180,T0+L*11000);
  const P11=P0*Math.pow(T11/T0,-G0/(L*R_AIR));
  const P20=P11*Math.exp(-G0*9000/(R_AIR*T11));
  const L2=0.001;
  T=T11+L2*(h-20000);
  P=P20*Math.pow(T/T11,-G0/(L2*R_AIR));
 }
 const rho=P/(R_AIR*T);
 const a=Math.sqrt(GAMMA_AIR*R_AIR*T);
 const mu=sutherlandViscosity(T);
 return {T,P,rho,a,mu};
}

function interpolateAtmosphereProfile(rows:AtmospherePoint[],aslM:number):{T:number;P:number;rho:number;a:number;mu:number}{
 if(!rows.length)return us1976AtAsl(aslM);
 if(aslM<=rows[0].altitude){
  const r=rows[0],T=r.temperature+273.15,P=r.pressure;
  const rho=r.density??P/(R_AIR*T),a=r.speedOfSound??Math.sqrt(GAMMA_AIR*R_AIR*T),mu=r.dynamicViscosity??sutherlandViscosity(T);
  return {T,P,rho,a,mu};
 }
 const last=rows[rows.length-1];
 if(aslM>=last.altitude){
  const T=last.temperature+273.15,P=last.pressure;
  const rho=last.density??P/(R_AIR*T),a=last.speedOfSound??Math.sqrt(GAMMA_AIR*R_AIR*T),mu=last.dynamicViscosity??sutherlandViscosity(T);
  return {T,P,rho,a,mu};
 }
 for(let i=1;i<rows.length;i++){
  const r0=rows[i-1],r1=rows[i];
  if(aslM<=r1.altitude){
   const u=(aslM-r0.altitude)/Math.max(1e-6,r1.altitude-r0.altitude);
   const T=(r0.temperature+u*(r1.temperature-r0.temperature))+273.15;
   const P=r0.pressure+u*(r1.pressure-r0.pressure);
   const rho0=r0.density??r0.pressure/(R_AIR*(r0.temperature+273.15));
   const rho1=r1.density??r1.pressure/(R_AIR*(r1.temperature+273.15));
   const rho=rho0+u*(rho1-rho0);
   const a=Math.sqrt(GAMMA_AIR*R_AIR*T);
   const mu=sutherlandViscosity(T);
   return {T,P,rho,a,mu};
  }
 }
 return us1976AtAsl(aslM);
}

function windAtAltitude(env:Environment,aglM:number,aslM:number):{speed:number;direction:number}{
 const model=env.windModel??'constant';
 if(model==='profile'&&env.windProfile?.length){
  const rows=env.windProfile;
  if(aslM<=rows[0].altitude)return {speed:rows[0].speed,direction:rows[0].direction};
  const last=rows[rows.length-1];
  if(aslM>=last.altitude)return {speed:last.speed,direction:last.direction};
  for(let i=1;i<rows.length;i++){
   const r0=rows[i-1],r1=rows[i];
   if(aslM<=r1.altitude){
    const u=(aslM-r0.altitude)/Math.max(1e-6,r1.altitude-r0.altitude);
    return {
     speed:r0.speed+u*(r1.speed-r0.speed),
     direction:((r0.direction+u*(r1.direction-r0.direction))%360+360)%360,
    };
   }
  }
 }
 const zRef=Math.max(0.5,env.windReferenceHeight??10);
 const zMin=Math.max(0.1,env.windMinimumHeight??1);
 const z=Math.max(zMin,aglM);
 let speed=env.windSpeed;
 if(model==='powerLaw'){
  const alpha=env.windShearExponent??(1/7);
  speed=env.windSpeed*Math.pow(z/zRef,alpha);
 }else if(model==='logarithmic'){
  const z0=Math.max(1e-4,env.windRoughnessLength??0.03);
  speed=env.windSpeed*(Math.log(Math.max(z,z0*1.01)/z0)/Math.log(Math.max(zRef,z0*1.01)/z0));
 }
 const dirGrad=env.windDirectionGradient??0;
 const direction=((env.windDirection+dirGrad*(aglM/1000))%360+360)%360;
 return {speed:Math.max(0,speed),direction};
}

export function generateEnvironmentProfile(env:Environment,maxAltitudeAglM=5000,stepM=250):EnvironmentProfileSample[]{
 const safeMax=Math.min(86000,Math.max(100,maxAltitudeAglM));
 const safeStep=Math.max(10,stepM);
 const samples:EnvironmentProfileSample[]=[];
 for(let agl=0;agl<=safeMax+1e-6;agl+=safeStep){
  const altitudeAglM=Math.min(safeMax,Math.round(agl*100)/100);
  const altitudeAslM=env.altitude+altitudeAglM;
  const atm=env.atmosphereModel==='profile'&&env.atmosphereProfile?.length
   ?interpolateAtmosphereProfile(env.atmosphereProfile,altitudeAslM)
   :env.atmosphereModel==='standard1976'
    ?us1976AtAsl(altitudeAslM)
    :us1976AtAsl(altitudeAglM,env.temperature+273.15,env.pressure);
  const wind=windAtAltitude(env,altitudeAglM,altitudeAslM);
  const dirRad=wind.direction*Math.PI/180;
  // Meteorological wind direction is "coming from" north clockwise:
  const windEastMs=-wind.speed*Math.sin(dirRad);
  const windNorthMs=-wind.speed*Math.cos(dirRad);
  samples.push({
   altitudeAglM,
   altitudeAslM,
   temperatureK:atm.T,
   temperatureC:atm.T-273.15,
   pressurePa:atm.P,
   densityKgM3:atm.rho,
   speedOfSoundMs:atm.a,
   dynamicViscosityPas:atm.mu,
   windSpeedMs:wind.speed,
   windDirectionDeg:wind.direction,
   windEastMs,
   windNorthMs,
  });
 }
 return samples;
}

export function environmentProfileCsv(samples:EnvironmentProfileSample[]):string{
 const header='altitude_agl_m,altitude_asl_m,temperature_c,temperature_k,pressure_pa,density_kg_m3,speed_of_sound_m_s,dynamic_viscosity_pa_s,wind_speed_m_s,wind_direction_deg';
 const lines=samples.map(s=>[
  s.altitudeAglM.toFixed(1),
  s.altitudeAslM.toFixed(1),
  s.temperatureC.toFixed(2),
  s.temperatureK.toFixed(2),
  s.pressurePa.toFixed(2),
  s.densityKgM3.toFixed(5),
  s.speedOfSoundMs.toFixed(2),
  s.dynamicViscosityPas.toExponential(4),
  s.windSpeedMs.toFixed(3),
  s.windDirectionDeg.toFixed(1),
 ].join(','));
 return [header,...lines].join('\n');
}

export function serializeEnvironmentProfileJson(env:Environment):string{
 return JSON.stringify({
  schema:'starlab.environment-profile.v1',
  savedAt:new Date().toISOString(),
  environment:env,
 },null,2);
}

export function parseEnvironmentProfileJson(raw:string,current:Environment):Environment{
 let parsed:unknown;
 try{parsed=JSON.parse(raw)}catch{throw Error('Çevre profili JSON biçimi okunamadı.')}
 if(!parsed||typeof parsed!=='object')throw Error('Çevre profili geçersiz.');
 const record=parsed as {environment?:Partial<Environment>}&Partial<Environment>;
 const candidate=record.environment&&typeof record.environment==='object'?record.environment:record;
 return validateEnvironment({...current,...candidate});
}
