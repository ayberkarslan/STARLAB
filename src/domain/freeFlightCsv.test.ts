import {it,expect} from 'vitest';
import {parseResultCsv,resultCsv,type FlightSample} from './flight';
import {emptyProject} from './project';
import {initialFlight} from './flight';
import {parseStar,serializeStar} from './starFile';

it('keeps N/E and aerodynamic telemetry through CSV export and import',()=>{
 const samples:FlightSample[]=[
  {t:0,altitude:15,velocity:50,acceleration:9.8,downrange:0,north:0,east:0,railCleared:true,mach:.15,dynamicPressure:100,dragCoefficient:.4,liftCoefficient:0,momentCoefficient:0},
  {t:1,altitude:61,velocity:45,acceleration:7,downrange:10,north:8,east:6,railCleared:true,mach:.13,dynamicPressure:80,centerOfPressure:.3,dragCoefficient:.41,liftCoefficient:.02,momentCoefficient:-.01},
 ];
 expect(parseResultCsv(resultCsv(samples))).toEqual(samples);
});

it('keeps the free flight mode, source version and component result in a .star project',()=>{
 const project=emptyProject(),flight=initialFlight();project.flight=flight;
 const scenario=flight.cases[0];scenario.solverMode='free6';scenario.freeFlight={startAltitude:5,startSpeed:20,motorAge:.2,duration:3,motorExitArea:.0001,sourceAtmosphere:'archive',initialState:{roll:1,v:2,w:3,p:.1,q:.2,r:.3},windTimeline:[{t:0,north:1,east:0,down:0},{t:3,north:2,east:1,down:0}],tailInlets:{tail:{mach:1.8,pressurePa:100000}}};
 scenario.result={source:'solver',signature:'snapshot',solverId:'flight-linux-x64-test',samples:[{t:0,altitude:5,velocity:20,acceleration:1},{t:1,altitude:15,velocity:18,acceleration:2}],components:[{id:'nose',centerOfPressure:.2,dragCoefficient:.3,liftCoefficient:0,momentCoefficient:0,tailInletMethod:'local_supersonic'}]};
 const restored=parseStar(serializeStar(project)).flight!.cases[0];
 expect(restored.solverMode).toBe('free6');
 expect(restored.freeFlight).toEqual(scenario.freeFlight);
 expect(restored.result?.solverId).toBe('flight-linux-x64-test');
 expect(restored.result?.components?.[0].tailInletMethod).toBe('local_supersonic');
 expect(restored.result?.components?.[0].centerOfPressure).toBe(.2);
});
