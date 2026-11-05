import {describe,expect,it} from 'vitest';
import {emptyProject,type Part} from '../src/domain/project';
import {initialFlight} from '../src/domain/flight';
import {defaultDispersionSettings} from '../src/domain/landingZone';
import {fullMissionInputs} from '../src/engine/fullMissionInputs';
import {runLandingZoneDispersion} from './landingZone';

const part=(id:string,parentId:string,kind:Part['kind'],length:number,diameter:number,offset=0):Part=>({id,parentId,kind,name:id,length,diameter,offset,mass:0,material:'Test',inputs:{wallThickness:2,density:2700}});

describe('landing zone dispersion engine',()=>{
 it('computes reproducible 3D trajectories and 2D dispersion ellipses across Monte Carlo trials',async()=>{
  const project=emptyProject(),flight=initialFlight();
  project.parts.push(part('stage','rocket','stage',0,0),
   {...part('nose','stage','nose',300,60),geometry:{noseProfile:'conical',filled:true}},
   part('body','stage','body',1000,60),
   {...part('fins','body','fins',180,180,600),geometry:{finCount:3,tipChord:80,sweep:100,finCrossSection:'square'},inputs:{wallThickness:3,density:2700}},
   {...part('mount','body','motor',200,56,750),geometry:{innerDiameter:52}},
   {...part('chute','body','parachute',80,50,250),mass:90});
  flight.motors.push({id:'test-motor',name:'Test',manufacturer:'Test',source:'custom',diameter:50,length:200,totalMass:.5,propellantMass:.1,curve:[{t:0,thrust:0},{t:.1,thrust:500},{t:1,thrust:0}]});
  flight.configurations[0].mounts.mount={motorId:'test-motor',event:'launch',delay:0,offset:0};
  flight.configurations[0].recovery.chute={event:'apogee',delay:0,altitude:300,dragCoefficient:1.5,referenceArea:.8,inflationTime:1};
  flight.cases[0].environment.atmosphereModel='surface';
  flight.cases[0].environment.windSpeed=4;
  flight.cases[0].environment.windDirection=240;
  flight.cases[0].environment.railAngle=4;
  flight.cases[0].environment.railHeading=60;
  flight.cases[0].environment.maxTime=180;
  const {payload,issues}=fullMissionInputs(project,flight,flight.cases[0]);
  expect(issues).toEqual([]);
  const settings={...defaultDispersionSettings,sampleCount:6,seed:77,payloadEnabled:true,payloadMassKg:2.5,payloadChuteAreaM2:0.6,payloadChuteCd:0.8};
  const [first,second]=await Promise.all([
   runLandingZoneDispersion({payload,settings,signature:'test-sig'},new AbortController().signal),
   runLandingZoneDispersion({payload,settings,signature:'test-sig'},new AbortController().signal),
  ]);
  expect(first.points).toHaveLength(7);
  expect(first.points[0].nominal).toBe(true);
  expect(first.points.every(p=>p.trajectory.length>10)).toBe(true);
  expect(first.ellipse.semiMajor1Sigma).toBeGreaterThan(1);
  expect(first.ellipse.semiMinor1Sigma).toBeGreaterThan(0.1);
  expect(first.ellipse.cep50Radius).toBeGreaterThan(1);
  expect(first.ellipse).toEqual(second.ellipse);
  expect(first.bodies?.map(b=>b.id)).toEqual(['sustainer','payload']);
  expect(first.bodies?.[1].ellipse.cep50Radius).toBeGreaterThan(1);
 });
});
