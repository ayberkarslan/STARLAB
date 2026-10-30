import {aerodynamicInput,aeroUnits} from '../domain/aerodynamics';
import {validate,type Part,type Project} from '../domain/project';
import {initialFlight} from '../domain/flight';
import {portableProject} from '../domain/starFile';

export const inputUnits={
 aerodynamics:aeroUnits,
 parts:{length:'mm',diameter:'mm',offset:'mm',mass:'g',wallThickness:'mm',density:'kg/m³',shearModulus:'Pa',normalForceSlope:'1/rad',surfaceDensity:'kg/m²',linearDensity:'kg/m',centerOfMass:'mm',centerOfPressure:'mm',roughness:'µm',angleOfAttack:'deg',mach:'1',dragCoefficient:'1'},
 motors:{length:'mm',diameter:'mm',totalMass:'kg',propellantMass:'kg',time:'s',t:'s',thrust:'N',centerOfMass:'mm',inertiaXX:'kg·m²',inertiaYY:'kg·m²',inertiaZZ:'kg·m²'},
 environment:{latitude:'deg',longitude:'deg',altitude:'m',temperature:'°C',pressure:'Pa',windSpeed:'m/s',windDirection:'deg',railLength:'m',railAngle:'deg',railHeading:'deg',timeStep:'s',maxTime:'s',speedOfSound:'m/s',dynamicViscosity:'Pa·s',density:'kg/m³',speed:'m/s',direction:'deg',windReferenceHeight:'m',windMinimumHeight:'m',windShearExponent:'1',windRoughnessLength:'m',windDirectionGradient:'deg/km',turbulence:{rms:'m/s',alpha:'1',poles:'count',sampleRate:'Hz',seed:'integer'}},
 events:{delay:'s',altitude:'m',inflationTime:'s',dragCoefficient:'1',referenceArea:'m²',recoveredMass:'kg',targetVelocity:'m/s',maxDeploymentPressure:'Pa'},
 geometry:{innerDiameter:'mm',aftDiameter:'mm',shoulderLength:'mm',shoulderDiameter:'mm',shoulderThickness:'mm',tabHeight:'mm',tabLength:'mm',tipChord:'mm',span:'mm',sweep:'mm',canopyDiameter:'mm',cordLength:'mm',lineLength:'mm',instanceSeparation:'mm',radialOffset:'mm',points:'mm',cantAngle:'deg',rotation:'deg',noseParameter:'1',shoulderCapped:'boolean',finCount:'count',lineCount:'count',instanceCount:'count'},
 materials:{bulkDensity:'kg/m³',surfaceDensity:'kg/m²',linearDensity:'kg/m',shearModulus:'Pa'},
} as const;
export const inputConventions={design:{axis:'x',positive:'aft',centers:'component-local-leading-origin',offset:'parent-local-origin'},motor:{cg:'motor-local-leading-origin',inertia:'about-local-cg',axial:'x'},environment:{altitude:'mean-sea-level',windDirection:'meteorological-from-clockwise-from-north',dateTime:'UTC'},recovery:{altitude:'above-launch-site',altitudeEvent:'descending'}} as const;
export function partInput(part:Part){
 const i=part.inputs??{};
 return {...part,material:part.material==='Tanımlanmadı'||!part.material.trim()?null:part.material,
  materialData:part.materialData?{...part.materialData,shearModulus:part.materialData.shearModulus>0?part.materialData.shearModulus:null}:undefined,
  mass:i.massProvided===false?null:i.massProvided||part.mass>0?part.mass:null,
  inputs:{wallThickness:i.wallThickness??null,density:i.density??null,shearModulus:i.shearModulus??null,normalForceSlope:i.normalForceSlope??null,surfaceDensity:i.surfaceDensity??null,linearDensity:i.linearDensity??null,centerOfMass:i.centerOfMass??null,centerOfPressure:i.centerOfPressure??null,dragCoefficient:i.dragCoefficient??null,roughness:i.roughness??null,mach:i.mach??null,angleOfAttack:i.angleOfAttack??null,massScope:i.massScope??'component',notes:i.notes??''},
 };
}
export type PartInput=ReturnType<typeof partInput>;
/** Serialization boundary only: no physics, estimated values or unit conversion. */
export function createProjectInputs(project:Project){
 const portable=portableProject(validate(project));const flight=portable.flight??initialFlight();
 return structuredClone({format:'starlab.inputs',schemaVersion:1,units:inputUnits,coordinates:inputConventions.design,conventions:inputConventions,project:{name:portable.name,aerodynamics:aerodynamicInput(portable.aerodynamics),parts:portable.parts.map(partInput),flight:{motors:flight.motors,configurations:flight.configurations,cases:flight.cases.map(({result,...inputs})=>inputs)}}});
}
