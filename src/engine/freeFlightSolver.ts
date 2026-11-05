import type {Project} from '../domain/project';
import type {FlightWorkspace,SimulationCase,FlightSample,FlightComponentSnapshot} from '../domain/flight';
import {allMotors,validateFreeWind,validateSamples,validateTailInlets} from '../domain/flight';
import {rocketInputs} from './rocketInputs';
import {evaluateStructure} from './client';
import {projectFreeFlight,type FreeFlightProjection} from './freeFlightInputs';

const number=(text:string,label:string)=>{if(text.trim()==='')throw Error(`${label} eksik.`);const value=Number(text);if(!Number.isFinite(value))throw Error(`${label} geçersiz.`);return value};

export function parseFreeFlightHistory(history:string,components:string){
 const lines=history.trim().split(/\r?\n/),header=lines.shift()?.split(',')??[];
 const required=['t_s','n_m','e_m','down_m','u','v','w','Mach','Q_Pa','mass','FnetX_N','FnetY_N','FnetZ_N','CP_m','CD','CL','CmCG'];
 const column=Object.fromEntries(required.map(key=>[key,header.indexOf(key)]));
 if(required.some(key=>column[key]<0))throw Error('Yeni uçuş telemetrisi eksik sütun içeriyor.');
 const samples:FlightSample[]=lines.map(line=>{
  const cells=line.split(','),at=(key:string)=>number(cells[column[key]]??'',key),mass=at('mass');
  const north=at('n_m'),east=at('e_m'),down=at('down_m');
  const cp=cells[column.CP_m]?.trim();
  return {t:at('t_s'),altitude:-down,velocity:Math.hypot(at('u'),at('v'),at('w')),
   acceleration:Math.hypot(at('FnetX_N'),at('FnetY_N'),at('FnetZ_N'))/mass,
   north,east,downrange:Math.hypot(north,east),railCleared:true,mach:at('Mach'),dynamicPressure:at('Q_Pa'),
   ...(cp?{centerOfPressure:number(cp,'CP_m')}:{ }),dragCoefficient:at('CD'),liftCoefficient:at('CL'),momentCoefficient:at('CmCG')};
 });
 const firstTime=samples[0]?.t,componentLines=components.trim().split(/\r?\n/),componentHeader=componentLines.shift()?.split(',')??[];
 const find=(key:string)=>componentHeader.indexOf(key);
 if(['t_s','surface','CP_m','CD','CL','CmCG'].some(key=>find(key)<0))throw Error('Bileşen telemetrisi eksik sütun içeriyor.');
 const snapshot:FlightComponentSnapshot[]=componentLines.filter(line=>Number(line.split(',')[find('t_s')])===firstTime).map(line=>{
  const cells=line.split(','),cp=cells[find('CP_m')]?.trim();
  return {id:cells[find('surface')],...(cp?{centerOfPressure:number(cp,'CP_m')}:{ }),dragCoefficient:number(cells[find('CD')],'CD'),liftCoefficient:number(cells[find('CL')],'CL'),momentCoefficient:number(cells[find('CmCG')],'CmCG'),...(find('tail_inlet_method')>=0?{tailInletMethod:cells[find('tail_inlet_method')]}:{})};
 });
 if(snapshot.length>64)throw Error('Bileşen telemetrisi çok büyük.');
 return {samples:validateSamples(samples),components:snapshot};
}

function nativeValue(result:Awaited<ReturnType<typeof evaluateStructure>>,id:string){
 const state=result.snapshot.results[id]?.state;
 if(state?.status!=='ready'||typeof state.value!=='number'||!Number.isFinite(state.value))throw Error(`Yapı hesabı ${id} üretemedi.`);
 return state.value;
}

async function massState(project:Project,flight:FlightWorkspace,configurationId:string,time:number,signal:AbortSignal){
 const projection=rocketInputs(project,flight,configurationId,time);
 if(projection.issues.length)throw Error(projection.issues.join('\n'));
 const response=await evaluateStructure({protocolVersion:1,requestId:crypto.randomUUID(),sessionId:'free-flight-mass',model:'rocket',inputs:{},assembly:projection.assembly},signal);
 return {mass:nativeValue(response,'total_mass'),cg:nativeValue(response,'total_cg'),ix:nativeValue(response,'total_axial'),iy:nativeValue(response,'total_transverse')};
}

export function freeFlightIssues(project:Project,flight:FlightWorkspace,scenario:SimulationCase){
 const configuration=flight.configurations.find(item=>item.id===scenario.configurationId);
 if(!configuration)return ['Uçuş konfigürasyonu bulunamadı.'];
 const geometry=projectFreeFlight(project,flight,configuration,scenario),issues=[...geometry.issues];
 try{validateFreeWind(scenario.freeFlight?.windTimeline,scenario.freeFlight?.duration??0)}catch(error){issues.push((error as Error).message)}
 try{validateTailInlets(scenario.freeFlight?.tailInlets)}catch(error){issues.push((error as Error).message)}
 for(const [id,inlet] of Object.entries(scenario.freeFlight?.tailInlets??{})){
  if(!geometry.parts.some(part=>part.id===id&&part.kind===3))issues.push(`${id}: yerel giriş yalnız boattail parçasına bağlanır.`);
  if(inlet.mach===undefined||inlet.pressurePa===undefined)issues.push(`${id}: yerel Mach ve statik basınç birlikte gerekli.`);
 }
 if(scenario.solverMode==='free3'){
  const e=scenario.environment,windEast=-e.windSpeed*Math.sin(e.windDirection*Math.PI/180);
  if(Math.abs(windEast)>1e-9||Math.abs(e.railHeading)>1e-9)issues.push('3DOF kuzey–irtifa düzleminde çalışır; doğu rüzgârı ve farklı yön için 6DOF seç.');
  if(scenario.freeFlight?.windTimeline?.some(point=>Math.abs(point.east)>1e-9))issues.push('3DOF düzleminde doğu rüzgârı sıfır olmalı.');
  const state=scenario.freeFlight?.initialState;
  if(state&&(Math.abs(state.roll)+Math.abs(state.v)+Math.abs(state.p)+Math.abs(state.r)>1e-9))issues.push('3DOF düzleminde roll, yan hız, roll/yaw hızları sıfır olmalı.');
 }
 const steps=(scenario.freeFlight?.duration??0)/scenario.environment.timeStep;
 if(steps>19500||steps*(geometry.parts.length+1)>25000)issues.push('Bu koşul çok fazla uçuş veya bileşen örneği üretiyor; zaman adımını büyüt ya da süreyi kısalt.');
 const motor=allMotors(flight).find(item=>item.id===geometry.motorId);
 if(motor&&scenario.freeFlight&&scenario.freeFlight.duration<motor.curve.at(-1)!.t-scenario.freeFlight.motorAge)issues.push('Serbest uçuş süresi kalan motor yanışından kısa olamaz.');
 return [...new Set(issues)];
}

function inputText(projected:FreeFlightProjection,scenario:SimulationCase,motor:{curve:{t:number;thrust:number}[]},mass0:{mass:number;cg:number;ix:number;iy:number},mass1:{mass:number;cg:number;ix:number;iy:number}){
 const e=scenario.environment,q=scenario.freeFlight!;
 const direction=e.windDirection*Math.PI/180,windNorth=-e.windSpeed*Math.cos(direction),windEast=-e.windSpeed*Math.sin(direction);
 const burn=motor.curve.at(-1)!.t;
 const main=[e.timeStep,e.altitude,q.startAltitude,q.startSpeed,e.railAngle,e.railHeading,windNorth,windEast,0,q.duration,q.motorAge,burn,
  mass0.mass,mass1.mass,-mass0.cg,mass0.ix,mass0.iy,mass0.iy,mass1.ix,mass1.iy,mass1.iy,projected.diameter,q.motorExitArea,projected.baseOwner];
 const state=q.initialState;
 const initial=[state?1:0,state?.roll??0,q.startSpeed,state?.v??0,state?.w??0,state?.p??0,state?.q??0,state?.r??0];
 const wind=q.windTimeline??[];
 const inlets=Object.entries(q.tailInlets??{});
 const lines=[main.join(' '),initial.join(' '),String(wind.length),...wind.map(point=>[point.t,point.north,point.east,point.down].join(' ')),q.sourceAtmosphere??'standard',String(inlets.length),...inlets.map(([id,input])=>[id,input.mach,input.pressurePa].join(' ')),String(motor.curve.length),...motor.curve.map(point=>`${point.t} ${point.thrust}`),String(projected.parts.length),...projected.parts.map(p=>[p.id,p.kind,p.station,p.length,p.foreRadius,p.aftRadius,p.number,p.span,p.root,p.tip,p.sweep,p.thickness].join(' '))];
 return 'STARLAB_FREE_FLIGHT_V2\n'+lines.join('\n')+'\n';
}

export async function solveFreeFlight(project:Project,flight:FlightWorkspace,scenario:SimulationCase,signal:AbortSignal){
 const issues=freeFlightIssues(project,flight,scenario);if(issues.length)throw Error(issues.join('\n'));
 const configuration=flight.configurations.find(item=>item.id===scenario.configurationId)!;
 const projected=projectFreeFlight(project,flight,configuration,scenario),motor=allMotors(flight).find(item=>item.id===projected.motorId)!;
 const q=scenario.freeFlight!,burn=motor.curve.at(-1)!.t;
 const mass0=await massState(project,flight,configuration.id,q.motorAge,signal);
 const mass1=await massState(project,flight,configuration.id,burn,signal);
 if(Math.abs(mass0.cg-mass1.cg)>1e-6)throw Error('Bu model faz boyunca sabit CG istiyor. Motor yanışı CG’yi değiştiriyor; bu proje için serbest uçuş hesabı durduruldu.');
 if(mass0.mass<mass1.mass||mass1.mass<=0||[mass0.ix,mass0.iy,mass1.ix,mass1.iy].some(value=>value<=0))throw Error('Kütle veya atalet serbest uçuş için geçersiz.');
 const response=await fetch('/api/free-flight/run',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:scenario.solverMode==='free3'?'3':'6',input:inputText(projected,scenario,motor,mass0,mass1)}),signal});
 const payload=await response.json() as {history?:string;components?:string;buildId?:string;error?:string};
 if(!response.ok||typeof payload.history!=='string'||typeof payload.components!=='string'||typeof payload.buildId!=='string')throw Error(payload.error||'Serbest uçuş motoru yanıt vermedi.');
 return {...parseFreeFlightHistory(payload.history,payload.components),buildId:payload.buildId};
}
