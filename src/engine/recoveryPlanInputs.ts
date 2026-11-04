import type {Project} from '../domain/project';
import type {FlightConfiguration} from '../domain/flight';

/** B3 roketsim_cli.cpp recovery.devices input projection, without calculations. */
export function recoveryPlanInputs(project:Project,configuration:FlightConfiguration){
 const issues:string[]=[];
 const devices=project.parts.filter(part=>part.kind==='parachute').flatMap(part=>{
  const setting=configuration.recovery[part.id]??{event:'apogee' as const,delay:0,altitude:300};
  if(setting.event==='never')return [];
  if(setting.dragCoefficient===undefined||setting.referenceArea===undefined||setting.inflationTime===undefined){issues.push(`${part.name}: açık paraşüt Cd, referans alanı ve şişme süresi gerekli.`);return []}
  return [{id:part.id,name:part.name,device_type:setting.role??'generic',
   trigger:setting.event==='altitude'?'ALTITUDE_AGL_DESCENDING':'APOGEE',
   trigger_altitude_agl_m:setting.event==='altitude'?setting.altitude:0,
   deployment_delay_s:setting.delay,
   inflation_time_s:setting.inflationTime,
   Cd:setting.dragCoefficient,reference_area_m2:setting.referenceArea,enabled:true}];
 });
 return {devices,issues};
}
