import {afterAll,describe,it,expect,vi} from 'vitest';
import {EngineService} from './engine';
import {runFreeFlight} from './freeFlight';
import {emptyProject,type Part} from '../src/domain/project';
import {initialFlight} from '../src/domain/flight';
import type {Motor} from '../src/domain/motors';
import {solveFreeFlight} from '../src/engine/freeFlightSolver';

const service=new EngineService();
afterAll(()=>{service.close();vi.unstubAllGlobals()});
const part=(id:string,parentId:string,kind:Part['kind'],length:number,diameter:number,offset=0):Part=>({id,parentId,kind,name:id,length,diameter,offset,mass:0,material:'Test',inputs:{wallThickness:2,density:2700}});

describe('STARLAB design to supplied C++ flight model',()=>{
 it('uses native structure results and completes a bounded post-rail flight',async()=>{
  const project=emptyProject(),flight=initialFlight();project.flight=flight;
  project.parts.push(part('stage','rocket','stage',0,0),part('body','stage','body',1000,60),
   {...part('mount','body','motor',200,56,400),geometry:{innerDiameter:52}});
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
  expect(output.components.map(item=>item.id)).toEqual(['body']);
  expect(output.buildId).toMatch(/^flight-/);
 });
});
