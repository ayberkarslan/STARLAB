import type {Project} from '../domain/project';
import type {FlightWorkspace,SimulationCase} from '../domain/flight';
import {defaultDispersionSettings,validateDispersionSettings,validateLandingZoneResult,type LandingZoneResult} from '../domain/landingZone';
import {fullMissionInputs} from './fullMissionInputs';

export async function solveLandingZone(project:Project,flight:FlightWorkspace,scenario:SimulationCase,signature:string,signal:AbortSignal):Promise<LandingZoneResult>{
 const {payload,issues}=fullMissionInputs(project,flight,scenario);
 if(issues.length)throw Error(issues.join('\n'));
 const settings=validateDispersionSettings(scenario.dispersion??defaultDispersionSettings);
 const response=await fetch('/api/landing-zone/run',{
  method:'POST',
  headers:{'Content-Type':'application/json'},
  body:JSON.stringify({payload,settings,signature}),
  signal,
 });
 const value=await response.json() as LandingZoneResult&{error?:string};
 if(!response.ok||value.error)throw Error(value.error||'İniş bölgesi motoru yanıt vermedi.');
 return validateLandingZoneResult(value);
}
