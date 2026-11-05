import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdtemp,readFile,rm,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import { availableParallelism, tmpdir } from 'node:os';
import {join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {validateCoefficientTable,type AeroCoefficientRow} from '../src/domain/aerodynamics';
import {
 computeLandingEllipse,decimateTrajectory,gaussianPair,mulberry32,
 validateDispersionSettings,type LandingBodyZone,type LandingDispersionSettings,type LandingPoint,type LandingZoneResult,
} from '../src/domain/landingZone';

const execute=promisify(execFile);
const root=fileURLToPath(new URL('../',import.meta.url));

async function missionBinary(signal?:AbortSignal){
 if(process.env.STARLAB_B3_CLI)return process.env.STARLAB_B3_CLI;
 const result=await execute(process.execPath,['scripts/build-mission.mjs'],{cwd:root,signal,timeout:180000,maxBuffer:100000,windowsHide:true});
 return result.stdout.trim();
}

interface SeparationState {
 t:number;
 east:number;
 north:number;
 altitude:number;
 vEast:number;
 vNorth:number;
 vVertical:number;
 preTrajectory:{t:number;east:number;north:number;altitude:number}[];
}

function simulateSeparatedBody3D(
 start:SeparationState,
 massKg:number,
 chuteAreaM2:number,
 chuteCd:number,
 deployAtApogee:boolean,
 windSpeedRef:number,
 windFromDeg:number,
 shearExp:number,
 refHeightM:number,
){
 const g=9.80665,dt=0.05;
 const toRad=(windFromDeg+180)*Math.PI/180;
 const baseWindE=windSpeedRef*Math.sin(toRad),baseWindN=windSpeedRef*Math.cos(toRad);
 let t=start.t,east=start.east,north=start.north,alt=Math.max(0.01,start.altitude);
 let vE=start.vEast,vN=start.vNorth,vZ=start.vVertical;
 let apogee=alt;
 for(const p of start.preTrajectory)if(p.altitude>apogee)apogee=p.altitude;
 const traj=[...start.preTrajectory];
 const ballisticCdA=0.012;
 let steps=0;
 while(alt>0&&steps<24000){
  steps++;
  const hEff=Math.max(1,alt);
  const shear=shearExp>0?Math.pow(hEff/Math.max(1,refHeightM),shearExp):1;
  const wE=baseWindE*shear,wN=baseWindN*shear;
  const rho=1.225*Math.exp(-alt/8500);
  const relE=vE-wE,relN=vN-wN,relZ=vZ;
  const relSpeed=Math.max(0.05,Math.hypot(relE,relN,relZ));
  const chuteOpen=!deployAtApogee||vZ<=0.2;
  const cdA=chuteOpen&&chuteAreaM2>0?chuteAreaM2*chuteCd:ballisticCdA;
  const dragOverM=(0.5*rho*cdA*relSpeed)/Math.max(0.1,massKg);
  const aE=-dragOverM*relE;
  const aN=-dragOverM*relN;
  const aZ=-g-dragOverM*relZ;
  vE+=aE*dt;vN+=aN*dt;vZ+=aZ*dt;
  const nextAlt=alt+vZ*dt;
  if(nextAlt<=0){
   const frac=alt/Math.max(1e-6,alt-nextAlt);
   t+=dt*frac;east+=vE*dt*frac;north+=vN*dt*frac;alt=0;
   traj.push({t,east,north,altitude:0});
   break;
  }
  t+=dt;east+=vE*dt;north+=vN*dt;alt=nextAlt;
  if(alt>apogee)apogee=alt;
  if(steps%4===0)traj.push({t,east,north,altitude:alt});
 }
 return {
  east,north,apogee,flightTime:t,
  impactSpeed:Math.hypot(vE,vN,vZ),
  trajectory:decimateTrajectory(traj,110),
 };
}

function parseTrialCsv(csv:string,railHeadingDeg:number){
 const lines=csv.trim().split(/\r?\n/);
 if(lines.length<2)throw Error('B3 dağılım CSV çıktısı boş.');
 const header=lines[0].split(',').map(c=>c.trim());
 const idx=(name:string)=>header.indexOf(name);
 const iT=idx('t'),iAlt=idx('altitude_agl_m'),iDown=idx('downrange_m'),iVx=idx('velocity_downrange_ms'),iVz=idx('velocity_vertical_ms');
 const iWindE=idx('wind_east_ms'),iWindN=idx('wind_north_ms'),iAir=idx('airspeed_ms'),iMass=idx('rocket_total_mass_kg'),iDrag=idx('drag_force_N');
 const iRail=idx('on_launch_rail'),iRec=idx('recovery_deployed'),iStages=idx('attached_stage_count');
 if([iT,iAlt,iDown,iVx,iVz].some(i=>i<0))throw Error('B3 dağılım CSV sütunu eksik.');
 const az=railHeadingDeg*Math.PI/180,sinAz=Math.sin(az),cosAz=Math.cos(az);
 let crossPos=0,crossVel=0,prevT=0,apogee=-1;
 const trajectory:{t:number;east:number;north:number;altitude:number}[]=[];
 let lastSpeed=0,lastTime=0;
 let apogeeState:SeparationState|undefined,stageSepState:SeparationState|undefined;
 let initialStages=1;
 for(let r=1;r<lines.length;r++){
  const cells=lines[r].split(',');
  if(cells.length<5)continue;
  const t=Number(cells[iT]),alt=Math.max(0,Number(cells[iAlt])),downrange=Number(cells[iDown]);
  const vx=Number(cells[iVx]),vz=Number(cells[iVz]);
  if(!Number.isFinite(t)||!Number.isFinite(alt)||!Number.isFinite(downrange))continue;
  const dt=Math.max(0,t-prevT);prevT=t;
  const onRail=iRail>=0?Number(cells[iRail])===1:false;
  const deployed=iRec>=0?Number(cells[iRec])===1:false;
  const stages=iStages>=0?Number(cells[iStages])||1:1;
  if(r===1)initialStages=stages;
  const windE=iWindE>=0?Number(cells[iWindE])||0:0;
  const windN=iWindN>=0?Number(cells[iWindN])||0:0;
  const crossWind=windE*cosAz-windN*sinAz;
  if(!onRail&&dt>0){
   const mass=iMass>=0?Math.max(0.05,Number(cells[iMass])||1):1;
   const airspeed=iAir>=0?Math.max(0.5,Number(cells[iAir])||1):Math.max(0.5,Math.hypot(vx,vz));
   const dragForce=iDrag>=0?Math.max(0,Number(cells[iDrag])||0):0;
   const rate=deployed?Math.max(1.8,9.80665/Math.max(Math.abs(vz),1)):dragForce/(mass*airspeed);
   const alpha=1-Math.exp(-rate*dt);
   crossVel+=(crossWind-crossVel)*alpha;
   crossPos+=crossVel*dt;
  }
  const east=downrange*sinAz+crossPos*cosAz;
  const north=downrange*cosAz-crossPos*sinAz;
  const vEast=vx*sinAz+crossVel*cosAz;
  const vNorth=vx*cosAz-crossVel*sinAz;
  trajectory.push({t,east,north,altitude:alt});
  if(alt>apogee){
   apogee=alt;
   apogeeState={t,east,north,altitude:alt,vEast,vNorth,vVertical:0,preTrajectory:decimateTrajectory(trajectory,45)};
  }
  if(!stageSepState&&initialStages>1&&stages<initialStages){
   stageSepState={t,east,north,altitude:alt,vEast,vNorth,vVertical:vz,preTrajectory:decimateTrajectory(trajectory,35)};
  }
  lastSpeed=Math.hypot(vx,vz,crossVel);
  lastTime=t;
 }
 const final=trajectory.at(-1)??{t:0,east:0,north:0,altitude:0};
 const fallbackState:SeparationState={t:lastTime,east:final.east,north:final.north,altitude:Math.max(0,apogee),vEast:0,vNorth:0,vVertical:0,preTrajectory:decimateTrajectory(trajectory,40)};
 return {
  east:final.east,
  north:final.north,
  apogee:Math.max(0,apogee),
  flightTime:lastTime,
  impactSpeed:lastSpeed,
  trajectory:decimateTrajectory(trajectory,110),
  apogeeState:apogeeState??fallbackState,
  stageSepState,
 };
}

export async function runLandingZoneDispersion(input:unknown,signal:AbortSignal):Promise<LandingZoneResult>{
 if(!input||typeof input!=='object'||Array.isArray(input))throw Error('İniş bölgesi isteği geçersiz.');
 const request=input as {payload?:unknown;settings?:unknown;signature?:string};
 if(!request.payload||typeof request.payload!=='object'||Array.isArray(request.payload))throw Error('Tam görev girdisi eksik.');
 const settings=validateDispersionSettings(request.settings);
 const serialized=JSON.stringify(request.payload);
 if(serialized.length>1_000_000)throw Error('İniş bölgesi girdisi çok büyük.');
 const binary=await missionBinary(signal),data=await readFile(binary),buildId=createHash('sha256').update(data).digest('hex').slice(0,16);
 const directory=await mkdtemp(join(tmpdir(),'starlab-landing-'));
 try{
  const basePayload=JSON.parse(serialized) as {
   motor_instances?:{id:string;stage_id?:string;wet_mass_kg:number;dry_mass_kg?:number;thrust_n:number[];cg_z_m?:number;transverse_inertia_per_mass_kgm2_per_kg?:number}[];
   stages?:{id:string;component_ids:string[];motor_instance_ids:string[]}[];
   recovery?:{recovery_enabled?:boolean;devices?:{cd_open?:number;area_m2?:number}[]};
   rasaero_table?:AeroCoefficientRow[];
   drag_settings?:{roughness_m?:number;perfect_finish?:boolean};
   simulation:Record<string,number|string|boolean>;
  };
  const analyzeIn=join(directory,'analyze-in.json'),analyzeOut=join(directory,'analysis.json');
  await writeFile(analyzeIn,serialized,'utf8');
  await execute(binary,['--analyze',analyzeIn,analyzeOut],{signal,timeout:60000,maxBuffer:100000,windowsHide:true});
  const design=JSON.parse(await readFile(analyzeOut,'utf8')) as {components?:{id:string;type:string;mass_kg:number;absolute_cg_m:number;inertia_transverse_kgm2:number}[]};
  if(!Array.isArray(design.components)||!Array.isArray(basePayload.motor_instances))throw Error('B3 tasarım veya motor analizi eksik.');
  for(const motor of basePayload.motor_instances){
   const component=design.components.find(item=>item.id===`${motor.id}:installed`&&item.type==='Motor');
   if(!component||!Number.isFinite(component.mass_kg)||component.mass_kg<=0)throw Error(`${motor.id}: B3 motor analizi uyuşmuyor.`);
   motor.cg_z_m=component.absolute_cg_m;
   motor.transverse_inertia_per_mass_kgm2_per_kg=component.inertia_transverse_kgm2/component.mass_kg;
  }
  if(basePayload.rasaero_table?.length){
   validateCoefficientTable(basePayload.rasaero_table);
   const rasaero=join(directory,'rasaero.csv');
   const rows=['Mach,Reynolds,AoA_deg,CA_off,CA_on,CN,CP_m,CN_alpha_per_rad',...basePayload.rasaero_table.map(r=>`${r.Mach},${r.Reynolds},${r.AoA_deg},${r.CA_off},${r.CA_on},${r.CN},${r.CP_m},${r.CN_alpha_per_rad}`)];
   await writeFile(rasaero,rows.join('\n'),'utf8');
   basePayload.simulation={...basePayload.simulation,rasaero_aero_table_path:rasaero};
   delete basePayload.rasaero_table;
  }
  const sim=basePayload.simulation;
  const baseWindSpeed=Number(sim.wind_speed_ms)||0;
  const baseWindDir=Number(sim.wind_direction_from_deg)||0;
  const baseRailAngle=Number(sim.launch_angle_deg)||0;
  const baseRailHeading=Number(sim.launch_azimuth_deg)||0;
  const baseRoughness=basePayload.drag_settings?.roughness_m??2e-5;
  const baseStepMs=Math.max(Number(sim.step_size_ms)||10,20);
  const shearExp=Number(sim.wind_shear_exponent)||0;
  const refHeightM=Number(sim.wind_reference_height_m)||10;
  const rng=mulberry32(settings.seed);
  const massSigma=settings.massScaleSigma??1.5;
  const cgSigmaM=(settings.cgOffsetSigmaMm??8)/1000;
  const chuteSigma=settings.parachuteCdSigma??6;
  const hasMultiStage=(basePayload.stages?.length??0)>1;
  const boosterStage=hasMultiStage?basePayload.stages?.at(-1):undefined;
  const boosterDryMassKg=boosterStage
   ?Math.max(0.5,design.components
    .filter(c=>boosterStage.component_ids.includes(c.id))
    .reduce((sum,c)=>sum+(Number.isFinite(c.mass_kg)?c.mass_kg:0),0))
   :2.5;

  const trials:{index:number;nominal:boolean;windSpeed:number;windDirection:number;railAngle:number;railHeading:number;thrustScale:number;dragScale:number;massScale:number;cgOffsetM:number;chuteScale:number}[]=[
   {index:0,nominal:true,windSpeed:baseWindSpeed,windDirection:baseWindDir,railAngle:baseRailAngle,railHeading:baseRailHeading,thrustScale:1,dragScale:1,massScale:1,cgOffsetM:0,chuteScale:1},
  ];
  for(let k=1;k<=settings.sampleCount;k++){
   const [z0,z1]=gaussianPair(rng),[z2,z3]=gaussianPair(rng),[z4,z5]=gaussianPair(rng),[z6,z7]=gaussianPair(rng),[z8]=gaussianPair(rng);
   const windSpeed=Math.max(0,baseWindSpeed+z0*settings.windSpeedSigma);
   const windDirection=((baseWindDir+z1*settings.windDirectionSigma)%360+360)%360;
   let railAngle:number,railHeading:number;
   if(baseRailAngle>0.5){
    railAngle=Math.min(85,Math.max(0.1,baseRailAngle+z2*settings.railAngleSigma));
    railHeading=((baseRailHeading+z3*settings.railHeadingSigma)%360+360)%360;
   }else{
    const tiltE=z2*settings.railAngleSigma,tiltN=z3*settings.railAngleSigma;
    railAngle=Math.min(85,Math.max(0.05,Math.hypot(tiltE,tiltN)));
    railHeading=((Math.atan2(tiltE,tiltN)*180/Math.PI)%360+360)%360;
   }
   const thrustScale=Math.min(1.5,Math.max(0.5,1+(z4*settings.thrustScaleSigma)/100));
   const dragScale=Math.min(2.5,Math.max(0.25,1+(z5*settings.dragScaleSigma)/100));
   const massScale=Math.min(1.35,Math.max(0.65,1+(z6*massSigma)/100));
   const cgOffsetM=Math.max(-0.2,Math.min(0.2,z7*cgSigmaM));
   const chuteScale=Math.min(1.6,Math.max(0.4,1+(z8*chuteSigma)/100));
   trials.push({index:k,nominal:false,windSpeed,windDirection,railAngle,railHeading,thrustScale,dragScale,massScale,cgOffsetM,chuteScale});
  }

  const results:LandingPoint[]=new Array(trials.length);
  const payloadResults:LandingPoint[]=new Array(trials.length);
  const boosterResults:LandingPoint[]=new Array(trials.length);
  const concurrency=Math.min(6,Math.max(2,availableParallelism()));
  let cursor=0;
  const worker=async()=>{
   while(cursor<trials.length){
    const idx=cursor++;
    const trial=trials[idx];
    const trialPayload=structuredClone(basePayload);
    trialPayload.simulation.step_size_ms=trial.nominal?Number(sim.step_size_ms)||10:baseStepMs;
    trialPayload.simulation.wind_speed_ms=trial.windSpeed;
    trialPayload.simulation.wind_direction_from_deg=trial.windDirection;
    trialPayload.simulation.launch_angle_deg=trial.railAngle;
    trialPayload.simulation.launch_azimuth_deg=trial.railHeading;
    trialPayload.simulation.turbulence_seed=(Number(sim.turbulence_seed)||1)+trial.index;
    if(!trial.nominal){
     trialPayload.drag_settings={
      roughness_m:Math.max(1e-7,baseRoughness*trial.dragScale*trial.dragScale),
      perfect_finish:false,
     };
     for(const m of trialPayload.motor_instances??[]){
      m.thrust_n=m.thrust_n.map(f=>f*trial.thrustScale);
      m.wet_mass_kg=Math.max(0.05,m.wet_mass_kg*trial.massScale);
      if(m.dry_mass_kg!==undefined)m.dry_mass_kg=Math.max(0.02,m.dry_mass_kg*trial.massScale);
      if(m.cg_z_m!==undefined)m.cg_z_m=Math.max(0.01,m.cg_z_m+trial.cgOffsetM);
     }
     for(const dev of trialPayload.recovery?.devices??[]){
      if(dev.cd_open!==undefined)dev.cd_open=Math.max(0.1,dev.cd_open*trial.chuteScale);
     }
    }
    const inFile=join(directory,`in-${trial.index}.json`);
    const csvFile=join(directory,`out-${trial.index}.csv`);
    await writeFile(inFile,JSON.stringify(trialPayload),'utf8');
    await execute(binary,[inFile,csvFile],{signal,timeout:90000,maxBuffer:200000,windowsHide:true});
    const parsed=parseTrialCsv(await readFile(csvFile,'utf8'),trial.railHeading);
    const makePoint=(p:{east:number;north:number;apogee:number;flightTime:number;impactSpeed:number;trajectory:{t:number;east:number;north:number;altitude:number}[]}):LandingPoint=>({
     index:trial.index,
     ...(trial.nominal?{nominal:true}:{}),
     east:Number(p.east.toFixed(3)),
     north:Number(p.north.toFixed(3)),
     apogee:Number(p.apogee.toFixed(2)),
     flightTime:Number(p.flightTime.toFixed(2)),
     impactSpeed:Number(p.impactSpeed.toFixed(2)),
     windSpeed:Number(trial.windSpeed.toFixed(3)),
     windDirection:Number(trial.windDirection.toFixed(2)),
     railAngle:Number(trial.railAngle.toFixed(3)),
     railHeading:Number(trial.railHeading.toFixed(2)),
     thrustScale:Number(trial.thrustScale.toFixed(4)),
     dragScale:Number(trial.dragScale.toFixed(4)),
     trajectory:p.trajectory,
    });
    results[idx]=makePoint(parsed);
    if(settings.payloadEnabled){
     const pMass=(settings.payloadMassKg??4)*trial.massScale;
     const pArea=settings.payloadChuteAreaM2??0.95;
     const pCd=(settings.payloadChuteCd??0.8)*trial.chuteScale;
     const simPayload=simulateSeparatedBody3D(parsed.apogeeState,pMass,pArea,pCd,false,trial.windSpeed,trial.windDirection,shearExp,refHeightM);
     payloadResults[idx]=makePoint(simPayload);
    }
    if(hasMultiStage&&parsed.stageSepState){
     const bMass=boosterDryMassKg*trial.massScale;
     const bArea=settings.boosterChuteAreaM2??1.2;
     const bCd=(settings.boosterChuteCd??0.8)*trial.chuteScale;
     const simBooster=simulateSeparatedBody3D(parsed.stageSepState,bMass,bArea,bCd,true,trial.windSpeed,trial.windDirection,shearExp,refHeightM);
     boosterResults[idx]=makePoint(simBooster);
    }
   }
  };
  await Promise.all(Array.from({length:concurrency},()=>worker()));
  const mainEllipse=computeLandingEllipse(results);
  const bodies:LandingBodyZone[]=[
   {id:'sustainer',name:hasMultiStage?'Üst kademe (Ana gövde)':'Ana gövde',ellipse:mainEllipse,points:results},
  ];
  if(settings.payloadEnabled&&payloadResults.every(Boolean)){
   bodies.push({id:'payload',name:'Görev yükü (Payload)',ellipse:computeLandingEllipse(payloadResults),points:payloadResults});
  }
  if(hasMultiStage&&boosterResults.every(Boolean)){
   bodies.push({id:'booster',name:'Alt kademe (Booster)',ellipse:computeLandingEllipse(boosterResults),points:boosterResults});
  }
  return {
   signature:typeof request.signature==='string'?request.signature:'',
   solverId:`b3-${buildId}`,
   settings,
   ellipse:mainEllipse,
   points:results,
   bodies,
  };
 }finally{
  await rm(directory,{recursive:true,force:true});
 }
}
