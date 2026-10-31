import type {Project} from '../domain/project';
import type {FlightWorkspace} from '../domain/flight';
import {resolveParts} from '../domain/geometry';
import {rocketInputs} from './rocketInputs';
import type {AssemblyPart} from './protocol';

const noseProfiles:Record<string,number>={conical:0,tangentOgive:1,elliptical:2,parabolic:3,power:4,haack:5,vonKarman:5};
const radians=(degrees:number)=>degrees*Math.PI/180;

/** Project data to the legacy static-design ports. No stability arithmetic runs here. */
export function designInputs(project:Project,flight:FlightWorkspace){
 const configurationId=flight.activeConfigurationId??flight.configurations[0]?.id;
 const structure=rocketInputs(project,flight,configurationId,0,{allowMultipleStages:true,ignoreSeparations:true}),issues=[...structure.issues],aeroAssembly:AssemblyPart[]=[];
 const explicit=project.aerodynamics?.referenceDiameter;
 const exposed=project.parts.filter(p=>['nose','body','transition'].includes(p.kind));
 const referenceDiameter=(explicit??Math.max(60,...exposed.flatMap(p=>[p.diameter,p.geometry?.aftDiameter??0])))/1000;
 const referenceRadius=referenceDiameter/2,referenceArea=Math.PI*referenceRadius*referenceRadius;
 const mach=project.aerodynamics?.mach??0,alpha=radians(project.aerodynamics?.angleOfAttack??0);
 for(const {part,position} of resolveParts(project)){
  const common={cnaOverrideEnabled:Number(part.inputs?.normalForceSlope!==undefined),cnaOverride:part.inputs?.normalForceSlope??0,cpOverrideEnabled:Number(part.inputs?.centerOfPressure!==undefined),cpOverride:(part.inputs?.centerOfPressure??0)/1000};
  if(part.kind==='nose'){
   const profile=part.geometry?.noseProfile?noseProfiles[part.geometry.noseProfile]:undefined;
   if(profile===undefined){issues.push(`${part.name}: statik aero için desteklenen burun profili seç.`);continue}
   aeroAssembly.push({id:`aero:${part.id}`,model:'nose-static-aero',position:position[0]/1000,inputs:{length:part.length/1000,radius:part.diameter/2000,profile,referenceRadius,...common}});
  }else if(part.kind==='fins'){
   const parent=project.parts.find(p=>p.id===part.parentId),g=part.geometry??{};
   if(parent?.kind!=='body'){issues.push(`${part.name}: statik aero için kanadı gövde tüpüne bağla.`);continue}
   const span=(part.diameter-parent.diameter)/2000;
   if(span<=0||g.tipChord===undefined||g.sweep===undefined||g.finCount===undefined){issues.push(`${part.name}: statik aero geometrisi eksik.`);continue}
   aeroAssembly.push({id:`aero:${part.id}`,model:'fins-static-aero',position:position[0]/1000,inputs:{root:part.length/1000,tip:g.tipChord/1000,span,sweep:g.sweep/1000,count:g.finCount,bodyRadius:parent.diameter/2000,referenceArea,mach,alpha,...common}});
  }else if(part.kind==='transition'){
   const aft=part.geometry?.aftDiameter;
   if(aft===undefined){issues.push(`${part.name}: statik aero için arka çap gerekli.`);continue}
   aeroAssembly.push({id:`aero:${part.id}`,model:'transition-static-aero',position:position[0]/1000,inputs:{length:part.length/1000,foreRadius:part.diameter/2000,aftRadius:aft/2000,referenceRadius,...common}});
  }else if(part.kind==='freeformFins'){
   const parent=project.parts.find(p=>p.id===part.parentId),g=part.geometry??{};
   if(parent?.kind!=='body'||!g.points||g.points.length<3||!g.finCount){issues.push(`${part.name}: statik aero geometrisi eksik.`);continue}
   aeroAssembly.push({id:`aero:${part.id}`,model:'freeform-static-aero',position:position[0]/1000,inputs:{points:g.points.flatMap(([x,y])=>[x/1000,y/1000]),count:g.finCount,bodyRadius:parent.diameter/2000,referenceArea,mach,alpha,...common}});
  }else if(part.kind==='tubeFins')issues.push(`${part.name}: statik aero modeli henüz bağlı değil.`);
 }
 if(!aeroAssembly.length)issues.push('CP için burun veya trapez kanat ekle.');
 return {assembly:structure.assembly,aeroAssembly,inputs:{referenceDiameter},issues,configurationId};
}
