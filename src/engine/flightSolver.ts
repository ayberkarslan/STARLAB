import {aerodynamicInput} from '../domain/aerodynamics';
import type {Project} from '../domain/project';
import type {FlightConfiguration,Environment,FlightSample} from '../domain/flight';
import type {Motor} from '../domain/motors';
import {allMotors,simulationSignature,type FlightWorkspace,type SimulationCase} from '../domain/flight';
import {partInput,inputUnits,inputConventions,type PartInput} from './projectInputs';
import {rocketInputs} from './rocketInputs';
import {evaluateStructure} from './client';
import type {AssemblyPart,EngineRequest} from './protocol';
export interface FlightRequest {version:1;signature:string;units:typeof inputUnits;conventions:typeof inputConventions;parts:PartInput[];assembly:AssemblyPart[];issues:string[];configuration:FlightConfiguration;environment:Environment;motors:Motor[];aerodynamics:ReturnType<typeof aerodynamicInput>}
export interface FlightSolver {solve(request:FlightRequest,signal:AbortSignal):Promise<{signature:string;samples:FlightSample[]}>}
// The team-supplied flight solver plugs in here. No placeholder trajectories.
let solver:FlightSolver|null=null;
export const getFlightSolver=()=>solver;
export const registerFlightSolver=(next:FlightSolver|null)=>{solver=next};
export function createFlightRequest(project:Project,flight:FlightWorkspace,scenario:SimulationCase):FlightRequest {
 const config=flight.configurations.find(c=>c.id===scenario.configurationId);if(!config)throw Error('Konfigürasyon bulunamadı.');
 const structure=rocketInputs(project,flight,config.id,0);
 return structuredClone({version:1,signature:simulationSignature(project,config,scenario.environment,flight),units:inputUnits,conventions:inputConventions,aerodynamics:aerodynamicInput(project.aerodynamics),parts:project.parts.map(partInput),assembly:structure.assembly,issues:structure.issues,configuration:{...config,separations:Object.fromEntries(project.parts.filter(p=>p.kind==='stage').map(p=>[p.id,config.separations[p.id]||{event:'never',delay:0}])),recovery:Object.fromEntries(project.parts.filter(p=>p.kind==='parachute').map(p=>[p.id,config.recovery[p.id]||{event:'apogee',delay:0,altitude:300}]))},environment:scenario.environment,motors:allMotors(flight).filter(m=>Object.values(config.mounts).some(a=>a.motorId===m.id))});
}

export function flightSolverIssues(request:FlightRequest){
 const issues=[...request.issues];
 if(request.motors.length!==1)issues.push('İlk uçuş çözücüsü tam olarak bir motor destekliyor.');
 if(Object.values(request.configuration.separations).some(s=>s.event!=='never'))issues.push('İlk uçuş çözücüsünde kademe ayrılması kapalı olmalı.');
 if(Object.values(request.configuration.recovery).some(r=>r.event!=='never'))issues.push('Kurtarma modeli sonraki uçuş diliminde bağlanacak.');
 const a=request.aerodynamics,e=request.environment;
 if(a.dragCoefficient===null||a.referenceArea===null)issues.push('Uçuş için eksenel Cd ve referans alanı gerekli.');
 if(e.atmosphereModel!=='surface'&&e.atmosphereModel!=='standard1976')issues.push('Yüzey veya 1976 standart atmosfer modelini seç.');
 if(e.windModel!==undefined&&e.windModel!=='constant')issues.push('İlk uçuş çözücüsü sabit rüzgâr destekliyor.');
 if(e.maxTime/e.timeStep>2_000_000)issues.push('Zaman adımı ve azami süre iki milyondan fazla integrasyon adımı üretiyor.');
 return [...new Set(issues)];
}

const nativeSolver:FlightSolver={async solve(request,signal){
 const issues=flightSolverIssues(request);if(issues.length)throw Error(issues.join('\n'));
 const a=request.aerodynamics,e=request.environment,m=request.motors[0];
 if(a.dragCoefficient===null||a.referenceArea===null)throw Error('Uçuş aero girdileri kayıp.');
 const engineRequest:EngineRequest={protocolVersion:1,requestId:crypto.randomUUID(),sessionId:'flight-solver',model:'flight',assembly:request.assembly,inputs:{curve:m.curve.flatMap(p=>[p.t,p.thrust]),propellant:m.propellantMass,cd:a.dragCoefficient,area:a.referenceArea,atmosphereModel:e.atmosphereModel==='surface'?0:1,launchAltitude:e.altitude,surfaceTemperature:e.temperature+273.15,surfacePressure:e.pressure,windSpeed:e.windSpeed,windDirection:e.windDirection,heading:e.railHeading,railAngle:e.railAngle,dt:e.timeStep,maxTime:e.maxTime}};
 const response=await evaluateStructure(engineRequest,signal),value=response.snapshot.results.flight_series?.state;
 if(!value)throw Error('Uçuş serisi üretilemedi.');
 if(value.status!=='ready')throw Error('error' in value?value.error:'Uçuş serisi üretilemedi.');
 if(!Array.isArray(value.value))throw Error('Uçuş serisi üretilemedi.');
 const samples:FlightSample[]=[];for(let i=0;i<value.value.length;i+=4)samples.push({t:value.value[i],altitude:value.value[i+1],velocity:value.value[i+2],acceleration:value.value[i+3]});
 return {signature:request.signature,samples};
}};
registerFlightSolver(nativeSolver);
