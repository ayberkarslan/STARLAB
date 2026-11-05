import type {Project} from '../domain/project';
import {parseResultCsv,type FlightEvent,type FlightWorkspace,type SimulationCase} from '../domain/flight';
import {fullMissionInputs} from './fullMissionInputs';

interface B3Event {type?:unknown;time_s?:unknown;altitude_agl_m?:unknown;state?:{altitude_m?:unknown};motor_id?:unknown;stage_id?:unknown;device_id?:unknown}

export function parseMissionEvents(summary:{events?:unknown[]}):FlightEvent[]{
 if(!Array.isArray(summary.events)||summary.events.length>1000)throw Error('B3 olay listesi geçersiz.');
 return summary.events.map(raw=>{
  const event=raw as B3Event,t=event.time_s,type=event.type;
  if(typeof type!=='string'||type.length>100||typeof t!=='number'||!Number.isFinite(t)||t<0)throw Error('B3 uçuş olayı geçersiz.');
  const altitude=event.altitude_agl_m??event.state?.altitude_m;
  const componentId=event.device_id??event.motor_id??event.stage_id;
  if(altitude!==undefined&&(typeof altitude!=='number'||!Number.isFinite(altitude))||componentId!==undefined&&typeof componentId!=='string')throw Error('B3 olay konumu geçersiz.');
  return {type,t,...(altitude===undefined?{}:{altitude}),...(componentId===undefined?{}:{componentId})} as FlightEvent;
 });
}

export async function solveFullMission(project:Project,flight:FlightWorkspace,scenario:SimulationCase,signal:AbortSignal){
 const {payload,issues}=fullMissionInputs(project,flight,scenario);if(issues.length)throw Error(issues.join('\n'));
 const response=await fetch('/api/full-mission/run',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload),signal});
 const value=await response.json() as {csv?:string;summary?:{events?:unknown[];landing?:{ground_impact_detected?:boolean}};buildId?:string;error?:string};
 if(!response.ok||typeof value.csv!=='string'||!value.summary||typeof value.buildId!=='string')throw Error(value.error||'B3 uçuş motoru yanıt vermedi.');
 let samples=parseResultCsv(value.csv);
 let events=parseMissionEvents(value.summary);
 const transonicSample=samples.find((s,i)=>i>0&&(samples[i-1].mach??0)<1&&(s.mach??0)>=1);
 if(transonicSample&&!events.some(e=>/transonik|mach 1/i.test(e.type))){
  events=[...events,{type:'Transonik bölge geçişi (Mach 1,0)',t:transonicSample.t,altitude:transonicSample.altitude}].sort((a,b)=>a.t-b.t);
 }
 if(scenario.environment.railAngle>=89.9&&!events.some(e=>/gimbal/i.test(e.type))){
  events=[{type:'Gimbal Lock Koruması (0,1° Sapma Yaması)',t:0,altitude:0},...events];
 }
 if(scenario.environment.stopCondition==='apogee'&&samples.length>2){
  let peakIdx=0;
  for(let i=1;i<samples.length;i++)if(samples[i].altitude>samples[peakIdx].altitude)peakIdx=i;
  if(peakIdx>=1){
   samples=samples.slice(0,peakIdx+1);
   const peakT=samples[peakIdx].t;
   events=events.filter(e=>e.t<=peakT+1e-3);
  }
 }
 return {samples,events,completed:scenario.environment.stopCondition==='apogee'?true:value.summary.landing?.ground_impact_detected===true,buildId:`b3-${value.buildId}`};
}
