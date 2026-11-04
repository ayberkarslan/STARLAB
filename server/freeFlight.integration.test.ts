import {afterAll,describe,it,expect,vi} from 'vitest';
import {EngineService} from './engine';
import {runFreeFlight} from './freeFlight';
import {emptyProject,type Part} from '../src/domain/project';
import {initialFlight} from '../src/domain/flight';
import type {Motor} from '../src/domain/motors';
import {solveFreeFlight} from '../src/engine/freeFlightSolver';
import {rocketInputs} from '../src/engine/rocketInputs';

const service=new EngineService();
afterAll(()=>{service.close();vi.unstubAllGlobals()});
const part=(id:string,parentId:string,kind:Part['kind'],length:number,diameter:number,offset=0):Part=>({id,parentId,kind,name:id,length,diameter,offset,mass:0,material:'Test',inputs:{wallThickness:2,density:2700}});

describe('STARLAB design to supplied C++ flight model',()=>{
 it('uses native structure results and completes a bounded post-rail flight',async()=>{
  const project=emptyProject(),flight=initialFlight();project.flight=flight;
  project.parts.push(part('stage','rocket','stage',0,0),
   {...part('nose','stage','nose',300,60),geometry:{noseProfile:'conical',filled:true},inputs:{density:2700}},
   part('body','stage','body',1000,60),
   {...part('fins','body','fins',180,180,600),geometry:{finCount:3,tipChord:80,sweep:100,finCrossSection:'square'},inputs:{wallThickness:3,density:2700}});
  const baseline=rocketInputs(project);expect(baseline.issues).toEqual([]);
  const dry=await service.evaluate({protocolVersion:1,requestId:'dry-cg',sessionId:'free-flight-fixture',model:'rocket',inputs:{},assembly:baseline.assembly});
  const cg=dry.snapshot.results.total_cg.state;
  if(cg.status!=='ready'||typeof cg.value!=='number')throw Error('Dry CG unavailable');
  project.parts.push({...part('mount','body','motor',200,56,cg.value*1000-400),geometry:{innerDiameter:52}});
  const motor:Motor={id:'balanced',name:'Balanced',manufacturer:'Test',source:'custom',diameter:50,length:200,totalMass:.5,propellantMass:.1,
   curve:[{t:0,thrust:0},{t:.1,thrust:50},{t:1,thrust:0}]};
  flight.motors.push(motor);flight.configurations[0].mounts.mount={motorId:motor.id,event:'launch',delay:0,offset:0};
  const scenario=flight.cases[0];scenario.solverMode='free6';scenario.environment.atmosphereModel='standard1976';
  scenario.freeFlight={startAltitude:5,startSpeed:15,motorAge:.1,duration:1.1,motorExitArea:.0001};
  vi.stubGlobal('fetch',async (url:string,options:{body:string;signal:AbortSignal})=>{
   if(url==='/api/calculations/structure'){
    const request=JSON.parse(options.body),result=await service.evaluate(request,()=>{},options.signal);
    return new Response(JSON.stringify(result)+'\n',{headers:{'content-type':'application/x-ndjson'}});
   }
   if(url==='/api/free-flight/run'){
    const request=JSON.parse(options.body),result=await runFreeFlight(request.mode,request.input,options.signal);
    return new Response(JSON.stringify(result),{headers:{'content-type':'application/json'}});
   }
   throw Error(`Unexpected endpoint ${url}`);
  });
  const output=await solveFreeFlight(project,flight,scenario,new AbortController().signal);
  expect(output.samples.length).toBeGreaterThan(10);
  expect(output.samples[0].altitude).toBeCloseTo(5);
  expect(output.samples.at(-1)!.altitude).toBeGreaterThan(5);
  expect(output.components.map(item=>item.id)).toEqual(['nose','body','fins']);
  expect(output.buildId).toMatch(/^flight-/);
 });
});
