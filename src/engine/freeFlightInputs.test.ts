import {describe,it,expect} from 'vitest';
import {addPart,emptyProject} from '../domain/project';
import {initialFlight} from '../domain/flight';
import {projectFreeFlight} from './freeFlightInputs';

function fixture(){
 let project=emptyProject();project=addPart(project,'rocket','stage');
 const stage=project.parts.at(-1)!.id;project=addPart(project,stage,'nose');const nose=project.parts.at(-1)!.id;
 project=addPart(project,stage,'body');const body=project.parts.at(-1)!.id;
 project=addPart(project,body,'fins');const fins=project.parts.at(-1)!.id;
 project=addPart(project,body,'motor');const mount=project.parts.at(-1)!.id;
 project={...project,parts:project.parts.map(part=>part.id===nose?{...part,geometry:{noseProfile:'conical'},diameter:60}:part.id===body?{...part,diameter:60}:part.id===fins?{...part,diameter:180,geometry:{finCount:3,tipChord:80,sweep:100,finCrossSection:'square'},inputs:{wallThickness:3}}:part)};
 const flight=initialFlight();flight.configurations[0].mounts[mount]={motorId:'catalog-estes-d12',event:'launch',delay:0,offset:0};
 flight.cases[0].solverMode='free6';flight.cases[0].freeFlight={startAltitude:5,startSpeed:15,motorAge:.1,duration:4,motorExitArea:.0001};
 return {project,flight};
}

describe('free flight projection',()=>{
 it('uses design stations and filters the motor mount out of external aero',()=>{
  const {project,flight}=fixture(),out=projectFreeFlight(project,flight,flight.configurations[0],flight.cases[0]);
  expect(out.issues).toEqual([]);
  expect(out.parts.map(part=>part.kind)).toEqual([0,1,4]);
  expect(out.parts[1].station).toBeCloseTo(.3);
  expect(out.parts[2].span).toBeCloseTo(.06);
 });
 it('rejects shapes not supplied by the new geometry model',()=>{
  const {project,flight}=fixture();project.parts.find(part=>part.kind==='nose')!.geometry={noseProfile:'tangentOgive'};
  expect(projectFreeFlight(project,flight,flight.configurations[0],flight.cases[0]).issues.join(' ')).toContain('konik burun');
 });
});
