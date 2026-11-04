import {describe,expect,it} from 'vitest';
import {emptyProject,type Part} from '../src/domain/project';
import {initialFlight,parseResultCsv} from '../src/domain/flight';
import {fullMissionInputs} from '../src/engine/fullMissionInputs';
import {runFullMission} from './fullMission';

const part=(id:string,parentId:string,kind:Part['kind'],length:number,diameter:number,offset=0):Part=>({id,parentId,kind,name:id,length,diameter,offset,mass:0,material:'Test',inputs:{wallThickness:2,density:2700}});

describe('B3 full mission bridge',()=>{
 it('runs a STARLAB single-stage launch through B3 to landing',async()=>{
  const project=emptyProject(),flight=initialFlight();
  project.parts.push(part('stage','rocket','stage',0,0),
   {...part('nose','stage','nose',300,60),geometry:{noseProfile:'conical',filled:true}},
   part('body','stage','body',1000,60),
   {...part('fins','body','fins',180,180,600),geometry:{finCount:3,tipChord:80,sweep:100,finCrossSection:'square'},inputs:{wallThickness:3,density:2700}},
   {...part('mount','body','motor',200,56,750),geometry:{innerDiameter:52}});
  flight.motors.push({id:'test-motor',name:'Test',manufacturer:'Test',source:'custom',diameter:50,length:200,totalMass:.5,propellantMass:.1,curve:[{t:0,thrust:0},{t:.1,thrust:500},{t:1,thrust:0}]});
  flight.configurations[0].mounts.mount={motorId:'test-motor',event:'launch',delay:0,offset:0};
  flight.cases[0].environment.atmosphereModel='surface';
  flight.cases[0].environment.maxTime=120;
  const {payload,issues}=fullMissionInputs(project,flight,flight.cases[0]);expect(issues).toEqual([]);
  const output=await runFullMission(payload,new AbortController().signal);
  const samples=parseResultCsv(output.csv);
  expect(samples.length).toBeGreaterThan(10);
  expect(Math.max(...samples.map(item=>item.altitude))).toBeGreaterThan(1);
  expect(output.summary.events).toEqual(expect.arrayContaining([expect.objectContaining({type:'ground_impact'})]));
 });
 it('runs a staged vehicle with a recovery device and source events',async()=>{
  const project=emptyProject(),flight=initialFlight();
  project.parts.push(part('upper-stage','rocket','stage',0,0),
   {...part('upper-nose','upper-stage','nose',300,60),geometry:{noseProfile:'conical',filled:true}},
   part('upper-body','upper-stage','body',800,60),
   {...part('upper-mount','upper-body','motor',200,56,580),geometry:{innerDiameter:52}},
   {...part('chute','upper-body','parachute',80,50,250),mass:90},
   part('lower-stage','rocket','stage',0,0),
   part('lower-body','lower-stage','body',700,60),
   {...part('lower-fins','lower-body','fins',180,180,450),geometry:{finCount:3,tipChord:80,sweep:100,finCrossSection:'square'},inputs:{wallThickness:3,density:2700}},
   {...part('lower-mount','lower-body','motor',200,56,470),geometry:{innerDiameter:52}});
  const motor={name:'Test',manufacturer:'Test',source:'custom' as const,diameter:50,length:200,totalMass:.5,propellantMass:.1,curve:[{t:0,thrust:0},{t:.1,thrust:500},{t:1,thrust:0}]};
  flight.motors.push({...motor,id:'first'},{...motor,id:'second'});
  const config=flight.configurations[0];
  config.mounts['lower-mount']={motorId:'first',event:'launch',delay:0,offset:0};
  config.mounts['upper-mount']={motorId:'second',event:'previousBurnout',delay:0,offset:0};
  config.separations['lower-stage']={event:'burnout',delay:0};
  config.recovery.chute={event:'apogee',delay:0,altitude:300,dragCoefficient:1.5,referenceArea:.8,inflationTime:1};
  flight.cases[0].environment.atmosphereModel='surface';flight.cases[0].environment.maxTime=120;
  const {payload,issues}=fullMissionInputs(project,flight,flight.cases[0]);expect(issues).toEqual([]);
  const output=await runFullMission(payload,new AbortController().signal);
  const samples=parseResultCsv(output.csv),types=(output.summary.events as {type:string}[]).map(event=>event.type);
  expect(samples.some(sample=>sample.attachedStageCount===1)).toBe(true);
  expect(types).toContain('STAGE_SEPARATION');
  expect(types).toContain('RECOVERY_TRIGGER');
  expect(types).toContain('ground_impact');
 });
});
