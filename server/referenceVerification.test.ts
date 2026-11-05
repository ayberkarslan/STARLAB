import {describe,expect,it} from 'vitest';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {mkdtemp,rm,writeFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {emptyProject} from '../src/domain/project';
import {builtInMotors} from '../src/domain/motors';
import {initialFlight,parseResultCsv} from '../src/domain/flight';
import {fullMissionInputs} from '../src/engine/fullMissionInputs';
import {handoffStateFromCase} from '../src/engine/freeFlightInputs';
import {freeFlightBuild} from './freeFlight';
import {runFullMission} from './fullMission';

const execute=promisify(execFile);

const ornekConfig=`setup 0.02 980 10 80 75 0 0 0 standard
phase birlesik 1.7 1.05 300 10 9.8 0 0.12 4 4 0.115 3.9 3.9
surface govde_burun 0.02 0.16 0.25 0 0 20
row 0 0.35 0.30 0.2 1.0 0 -1.0 0 0 0 0 0 0
row 0.8 0.40 0.35 0.3 1.1 0 -1.1 0 0 0 0 0 0
surface kanatcik_takimi 0.012 0.16 -0.65 0 0 20
row 0 0.18 0.18 0.4 3.0 0 -3.0 0 -0.4 0 -3 0 -3
row 0.8 0.22 0.22 0.5 3.2 0 -3.2 0 -0.5 0 -4 0 -4
phase ust 2.5 0.9 160 5 4.9 0.20 0.08 1.9 1.9 0.078 1.85 1.85
surface ust_govde 0.018 0.15 0.40 0 0 20
row 0 0.34 0.28 0.2 0.9 0 -0.9 0 0 0 0 0 0
row 0.8 0.38 0.32 0.3 1 0 -1 0 0 0 0 0 0
surface ust_kanatcik_takimi 0.012 0.15 -0.45 0 0 20
row 0 0.18 0.18 0.4 2.8 0 -2.8 0 -0.4 0 -3 0 -3
row 0.8 0.20 0.20 0.5 3.0 0 -3.0 0 -0.5 0 -4 0 -4
`;

const yanRuzgarConfig=`setup 0.02 980 10 80 75 0 0 0 standard
phase birlesik 1.7 1.05 300 10 9.8 0 0.12 4 4 0.115 3.9 3.9
motor 0 200
motor 0.4 350
motor 1.05 0
wind 0 0 1 0
wind 1.7 0 2 0
surface govde_burun 0.02 0.16 0.25 0 0 20
row 0 0.35 0.30 0.2 1.0 0 -1.0 0 0 0 0 0 0
row 0.8 0.40 0.35 0.3 1.1 0 -1.1 0 0 0 0 0 0
surface kanatcik_takimi 0.012 0.16 -0.65 0 0 20
row 0 0.18 0.18 0.4 3.0 0 -3.0 0 -0.4 0 -3 0 -3
row 0.8 0.22 0.22 0.5 3.2 0 -3.2 0 -0.5 0 -4 0 -4
phase ust 2.5 0.9 160 5 4.9 0.20 0.08 1.9 1.9 0.078 1.85 1.85
surface ust_govde 0.018 0.15 0.40 0 0 20
row 0 0.34 0.28 0.2 0.9 0 -0.9 0 0 0 0 0 0
row 0.8 0.38 0.32 0.3 1 0 -1 0 0 0 0 0 0
surface ust_kanatcik_takimi 0.012 0.15 -0.45 0 0 20
row 0 0.18 0.18 0.4 2.8 0 -2.8 0 -0.4 0 -3 0 -3
row 0.8 0.20 0.20 0.5 3.0 0 -3.0 0 -0.5 0 -4 0 -4
`;

function denemeBenchmark(){
 const project=emptyProject(),flight=initialFlight();
 project.parts.push(
  {id:'stage1',parentId:'rocket',kind:'stage',name:'Kademe',length:0,diameter:0,offset:0,mass:0,material:'Kompozit',inputs:{density:1500}},
  {id:'nose',parentId:'stage1',kind:'nose',name:'Burun konisi',length:600,diameter:150,offset:0,mass:0,material:'Kompozit',inputs:{wallThickness:3,density:1500},geometry:{noseProfile:'conical',filled:true,noseParameter:1}},
  {id:'body',parentId:'stage1',kind:'body',name:'Gövde borusu',length:2900,diameter:150,offset:0,mass:0,material:'Kompozit',inputs:{wallThickness:3,density:1500}},
  {id:'fins',parentId:'body',kind:'fins',name:'Trapez kanat takımı',length:450,diameter:300,offset:2450,mass:0,material:'Kompozit',inputs:{wallThickness:6,density:1500},geometry:{finCount:3,tipChord:200,sweep:160,finCrossSection:'square'}},
  {id:'mount',parentId:'body',kind:'motor',name:'Motor yuvası',length:620,diameter:104,offset:2255,mass:0,material:'Kompozit',inputs:{wallThickness:2,density:1500},geometry:{innerDiameter:100}},
 );
 const st98=builtInMotors.find(m=>m.id==='sample-98')!;
 flight.configurations[0].mounts.mount={motorId:st98.id,event:'launch',delay:0,offset:30};
 flight.configurations[0].separations.stage1={event:'never',delay:0};
 flight.cases[0].environment.atmosphereModel='surface';
 return {project,flight,burn:st98.curve.at(-1)!.t};
}

describe('reference verification library',()=>{
 it('reproduces ENTEGRASYON multi-phase surface Mach table and lateral wind fixtures',async()=>{
  const binary=await freeFlightBuild();
  const dir=await mkdtemp(join(tmpdir(),'starlab-ref-'));
  try{
   const ornekPath=join(dir,'ornek.txt'),yanPath=join(dir,'yan_ruzgar.txt');
   await writeFile(ornekPath,ornekConfig,'utf8');
   await writeFile(yanPath,yanRuzgarConfig,'utf8');
   const [ornek3,ornek6,yan6]=await Promise.all([
    execute(binary,['3',ornekPath]),
    execute(binary,['6',ornekPath]),
    execute(binary,['6',yanPath]),
   ]);
   const lastRow=(stdout:string)=>{
    const history=stdout.slice(10,stdout.indexOf('STCOMPONENTS\n')).trim().split(/\r?\n/);
    const header=history[0].split(','),last=history.at(-1)!.split(',');
    return (col:string)=>Number(last[header.indexOf(col)]);
   };
   const r3=lastRow(ornek3.stdout),r6=lastRow(ornek6.stdout),rw=lastRow(yan6.stdout);
   expect(r3('n_m')).toBeCloseTo(122.80483725827,8);
   expect(r3('down_m')).toBeCloseTo(-371.63807368993,8);
   expect(r6('n_m')).toBeCloseTo(r3('n_m'),10);
   expect(r6('down_m')).toBeCloseTo(r3('down_m'),10);
   expect(r6('mass')).toBeCloseTo(4.9,10);
   expect(rw('n_m')).toBeCloseTo(115.88423184883,8);
   expect(rw('e_m')).toBeCloseTo(-0.98826017862213,8);
   expect(rw('down_m')).toBeCloseTo(-346.98390638717,8);
  }finally{await rm(dir,{recursive:true,force:true});}
 });

 it('reproduces the DENEME OpenRocket vs STARLAB B3 benchmark and burnout handoff',async()=>{
  const smooth=denemeBenchmark(),normalFinish=denemeBenchmark(),openRocketMatch=denemeBenchmark();
  normalFinish.project.aerodynamics={surfaceRoughness:60};
  openRocketMatch.project.aerodynamics={surfaceRoughness:60};
  openRocketMatch.flight.cases[0].environment.windSpeed=2;
  const [outSmooth,outNormal,outOrk]=await Promise.all([
   runFullMission(fullMissionInputs(smooth.project,smooth.flight,smooth.flight.cases[0]).payload,new AbortController().signal),
   runFullMission(fullMissionInputs(normalFinish.project,normalFinish.flight,normalFinish.flight.cases[0]).payload,new AbortController().signal),
   runFullMission(fullMissionInputs(openRocketMatch.project,openRocketMatch.flight,openRocketMatch.flight.cases[0]).payload,new AbortController().signal),
  ]);
  const samplesSmooth=parseResultCsv(outSmooth.csv),samplesNormal=parseResultCsv(outNormal.csv),samplesOrk=parseResultCsv(outOrk.csv);
  expect(samplesSmooth[0].mass).toBeCloseTo(19.08226,4);
  expect(samplesSmooth.at(-1)!.mass).toBeCloseTo(15.50726,4);
  expect(samplesSmooth[0].centerOfGravity).toBeCloseTo(2.0663,3);
  const railExit=samplesSmooth.find(s=>s.railCleared)!;
  expect(railExit.velocity).toBeCloseTo(35.39,1);
  expect(Math.max(...samplesSmooth.map(s=>s.acceleration))).toBeCloseTo(152.8,1);
  const apogeeSmooth=Math.max(...samplesSmooth.map(s=>s.altitude));
  const apogeeNormal=Math.max(...samplesNormal.map(s=>s.altitude));
  const apogeeOrk=Math.max(...samplesOrk.map(s=>s.altitude));
  const maxVelOrk=Math.max(...samplesOrk.map(s=>s.velocity));
  expect(apogeeSmooth).toBeCloseTo(1932.0,0);
  expect(apogeeNormal).toBeCloseTo(1772.2,0);
  expect(apogeeOrk).toBeGreaterThan(1765);
  expect(apogeeOrk).toBeLessThan(1775);
  expect(maxVelOrk).toBeCloseTo(222.5,0);
  smooth.flight.cases[0].result={source:'solver',signature:'bench',samples:samplesSmooth};
  const handoff=handoffStateFromCase(smooth.flight.cases[0],smooth.burn,'burnout');
  expect(handoff).not.toBeNull();
  expect(handoff!.motorAge).toBeCloseTo(2.4,4);
  expect(handoff!.startSpeed).toBeGreaterThan(200);
 });
});
