import {validate,type Part,type Project} from '../domain/project';
import {initialFlight} from '../domain/flight';
import {portableProject} from '../domain/starFile';

export const inputUnits={
 parts:{length:'mm',diameter:'mm',offset:'mm',mass:'g',wallThickness:'mm',density:'kg/m³',surfaceDensity:'kg/m²',linearDensity:'kg/m',centerOfMass:'mm',centerOfPressure:'mm',roughness:'µm',angleOfAttack:'deg',mach:'1',dragCoefficient:'1'},
 motors:{length:'mm',diameter:'mm',totalMass:'kg',propellantMass:'kg',time:'s',thrust:'N'},
 environment:{latitude:'deg',longitude:'deg',altitude:'m',temperature:'°C',pressure:'Pa',windSpeed:'m/s',windDirection:'deg',railLength:'m',railAngle:'deg',railHeading:'deg',timeStep:'s',maxTime:'s'},
 events:{delay:'s',altitude:'m'},
 geometry:{innerDiameter:'mm',aftDiameter:'mm',shoulderLength:'mm',shoulderDiameter:'mm',shoulderThickness:'mm',tipChord:'mm',span:'mm',sweep:'mm',canopyDiameter:'mm',cordLength:'mm',lineLength:'mm',instanceSeparation:'mm',radialOffset:'mm',points:'mm',cantAngle:'deg',rotation:'deg',noseParameter:'1',finCount:'count',lineCount:'count',instanceCount:'count'},
 materials:{bulkDensity:'kg/m³',surfaceDensity:'kg/m²',linearDensity:'kg/m',shearModulus:'Pa'},
} as const;
export function partInput(part:Part){
 const i=part.inputs??{};
 return {...part,material:part.material==='Tanımlanmadı'||!part.material.trim()?null:part.material,
  materialData:part.materialData?{...part.materialData,shearModulus:part.materialData.shearModulus>0?part.materialData.shearModulus:null}:undefined,
  mass:i.massProvided===false?null:i.massProvided||part.mass>0?part.mass:null,
  inputs:{wallThickness:i.wallThickness??null,density:i.density??null,surfaceDensity:i.surfaceDensity??null,linearDensity:i.linearDensity??null,centerOfMass:i.centerOfMass??null,centerOfPressure:i.centerOfPressure??null,dragCoefficient:i.dragCoefficient??null,roughness:i.roughness??null,mach:i.mach??null,angleOfAttack:i.angleOfAttack??null,massScope:i.massScope??'component',notes:i.notes??''},
 };
}
export type PartInput=ReturnType<typeof partInput>;
/** Serialization boundary only: no physics, estimated values or unit conversion. */
export function createProjectInputs(project:Project){
 const portable=portableProject(validate(project));const flight=portable.flight??initialFlight();
 return structuredClone({format:'starlab.inputs',schemaVersion:1,units:inputUnits,coordinates:{axis:'x',positive:'aft',centers:'component-local-leading-origin',offset:'parent-local-origin'},project:{name:portable.name,parts:portable.parts.map(partInput),flight:{motors:flight.motors,configurations:flight.configurations,cases:flight.cases.map(({result,...inputs})=>inputs)}}});
}
