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
 return {samples:parseResultCsv(value.csv),events:parseMissionEvents(value.summary),completed:value.summary.landing?.ground_impact_detected===true,buildId:`b3-${value.buildId}`};
}
