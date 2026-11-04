/// <reference types="vite/client" />
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdtemp,readFile,rm,stat,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';

const execute=promisify(execFile);
const root=fileURLToPath(new URL('../',import.meta.url));
async function missionBinary(signal?:AbortSignal){
 if(process.env.STARLAB_B3_CLI)return process.env.STARLAB_B3_CLI;
 const result=await execute(process.execPath,['scripts/build-mission.mjs'],{cwd:root,signal,timeout:180000,maxBuffer:100000,windowsHide:true});
 return result.stdout.trim();
}

export async function fullMissionVersion(){
 const binary=await missionBinary();
 const data=await readFile(binary).catch(()=>{throw Error('B3 uçuş ikilisi bulunamadı. STARLAB_B3_CLI yolunu ayarla.');});
 return createHash('sha256').update(data).digest('hex').slice(0,16);
}

export async function runFullMission(input:unknown,signal:AbortSignal){
 if(!input||typeof input!=='object'||Array.isArray(input))throw Error('Tam görev girdisi geçersiz.');
 const serialized=JSON.stringify(input);if(serialized.length>1_000_000)throw Error('Tam görev girdisi çok büyük.');
 const binary=await missionBinary(signal),data=await readFile(binary),buildId=createHash('sha256').update(data).digest('hex').slice(0,16),directory=await mkdtemp(join(tmpdir(),'starlab-flight-'));
 const source=join(directory,'input.json'),analysis=join(directory,'analysis.json'),telemetry=join(directory,'flight.csv'),summary=join(directory,'summary.json');
 try{
  await writeFile(source,serialized,'utf8');
  await execute(binary,['--analyze',source,analysis],{signal,timeout:60000,maxBuffer:100000,windowsHide:true});
  const design=JSON.parse(await readFile(analysis,'utf8')) as {components?:{id:string;type:string;mass_kg:number;absolute_cg_m:number;inertia_transverse_kgm2:number}[]};
  const payload=JSON.parse(serialized) as {motor_instances?:{id:string;wet_mass_kg:number;cg_z_m?:number;transverse_inertia_per_mass_kgm2_per_kg?:number}[]};
  if(!Array.isArray(design.components)||!Array.isArray(payload.motor_instances))throw Error('B3 tasarım veya motor analizi eksik.');
  for(const motor of payload.motor_instances){
   const component=design.components.find(item=>item.id===`${motor.id}:installed`&&item.type==='Motor');
   if(!component||!Number.isFinite(component.mass_kg)||component.mass_kg<=0||Math.abs(component.mass_kg-motor.wet_mass_kg)>1e-6||!Number.isFinite(component.absolute_cg_m)||!Number.isFinite(component.inertia_transverse_kgm2))throw Error(`${motor.id}: B3 motor kütle/atalet analizi uyuşmuyor.`);
   motor.cg_z_m=component.absolute_cg_m;
   motor.transverse_inertia_per_mass_kgm2_per_kg=component.inertia_transverse_kgm2/component.mass_kg;
  }
  await writeFile(source,JSON.stringify(payload),'utf8');
  await execute(binary,[source,telemetry,summary],{signal,timeout:120000,maxBuffer:200000,windowsHide:true});
  if((await stat(telemetry)).size>32_000_000)throw Error('B3 uçuş sonucu çok büyük.');
  const events=JSON.parse(await readFile(summary,'utf8')) as {events?:unknown[];landing?:{ground_impact_detected?:boolean};staging_warnings?:unknown[]};
  if(!Array.isArray(events.events))throw Error('B3 olay özeti eksik.');
  return {csv:await readFile(telemetry,'utf8'),summary:events,buildId};
 }finally{await rm(directory,{recursive:true,force:true});}
}
