import type {Project} from '../domain/project';
import {resolveParts} from '../domain/geometry';
import {isStructural} from '../domain/componentCatalog';
import type {FlightConfiguration} from '../domain/flight';

/** Projects STARLAB's hierarchy/events onto B3 staging.hpp stage records. */
export function stagePlanInputs(project:Project,configuration:FlightConfiguration){
 const parts=new Map(project.parts.map(part=>[part.id,part]));
 const stageOf=(id:string):string|undefined=>{
  let part=parts.get(id);
  while(part&&part.kind!=='stage')part=part.parentId?parts.get(part.parentId):undefined;
  return part?.id;
 };
 const positions=new Map(resolveParts(project).map(item=>[item.part.id,item.position[0]]));
 const stages=project.parts.filter(part=>part.kind==='stage').sort((a,b)=>(positions.get(b.id)??0)-(positions.get(a.id)??0));
 return stages.map(stage=>{
  const event=configuration.separations[stage.id]??{event:'never',delay:0};
  return {
   id:stage.id,
   component_ids:project.parts.filter(part=>!isStructural(part.kind)&&part.kind!=='motor'&&stageOf(part.id)===stage.id).map(part=>part.id),
   motor_instance_ids:project.parts.filter(part=>part.kind==='motor'&&stageOf(part.id)===stage.id&&configuration.mounts[part.id]).map(part=>part.id),
   separation_type:event.event==='nextStageIgnition'?'next_stage_ignition':'own_motor_burnout',
   separation_delay_s:event.delay,
   separation_enabled:event.event!=='never',
  };
 });
}
