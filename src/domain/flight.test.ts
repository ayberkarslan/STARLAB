import {describe,it,expect} from 'vitest';
import {initialProject,validate} from './project';
import {initialFlight,validateFlight,parseResultCsv,configurationIssues,simulationSignature,cleanFlight} from './flight';
import {parseEng,exportEng,motorFits,motorStats,sampleMotors,catalogMotors} from './motors';
import {createFlightRequest} from '../engine/flightSolver';
describe('flight workspace',()=>{
 it('keeps old projects valid and round trips flight data',()=>{const project=initialProject();expect(validate(project)).toEqual(project);project.flight=initialFlight();expect(validate(JSON.parse(JSON.stringify(project)))).toEqual(project)});
 it('rejects invalid references and environment',()=>{const f=initialFlight();f.cases[0].configurationId='missing';expect(()=>validateFlight(f)).toThrow();f.cases[0].configurationId=f.configurations[0].id;f.cases[0].environment.timeStep=0;expect(()=>validateFlight(f)).toThrow()});
 it('detects unassigned and oversized motors',()=>{const p=initialProject(),f=initialFlight(),c=f.configurations[0];expect(configurationIssues(p,f,c)).toHaveLength(2);c.mounts.umotor={motorId:'sample-98',event:'launch',delay:0};expect(configurationIssues(p,f,c).join(' ')).toContain('uymuyor');expect(motorFits(sampleMotors[0],p.parts.find(p=>p.id==='umotor')!)).toBe(true)});
 it('invalidates results on meaningful input changes',()=>{const p=initialProject(),f=initialFlight(),c=f.configurations[0],e=f.cases[0].environment;const signature=simulationSignature(p,c,e,f);expect(simulationSignature(p,c,{...e,windSpeed:5},f)).not.toBe(signature);p.parts[3].length+=20;expect(simulationSignature(p,c,e,f)).not.toBe(signature)});
 it('removes assignments when mounts are deleted',()=>{const p=initialProject(),f=initialFlight();f.configurations[0].mounts.umotor={motorId:'sample-54',event:'launch',delay:0};p.parts=p.parts.filter(p=>p.id!=='umotor');expect(cleanFlight(p,f).configurations[0].mounts).toEqual({})});
 it('parses external results and rejects invalid time series',()=>{expect(parseResultCsv('t,altitude,velocity,acceleration\n0,0,0,0\n1,10,20,5')).toHaveLength(2);expect(()=>parseResultCsv('t,altitude,velocity,acceleration\n1,0,0,0\n1,0,0,0')).toThrow()});
});
describe('motor data',()=>{
 it('ships stable legacy ENG catalog motors',()=>{expect(catalogMotors.map(m=>[m.id,m.name,m.source])).toEqual([['catalog-estes-d12','D12','catalog'],['catalog-estes-e12','E12','catalog'],['catalog-aerotech-j270w','J270W','catalog']]);for(const motor of catalogMotors){expect(motor.curve[0]).toEqual({t:0,thrust:0});expect(motor.curve.at(-1)?.thrust).toBe(0);expect(motorStats(motor).impulse).toBeGreaterThan(0)}});
 it('exports and reimports RASP curves without changing their numbers',()=>{const m=sampleMotors[0],loaded=parseEng(exportEng(m))[0];expect(loaded.curve).toEqual(m.curve);expect(motorStats(loaded)).toEqual(motorStats(m));expect(loaded.diameter).toBe(m.diameter)});
 it('imports multiple RASP motors with mass in kg',()=>{const m=parseEng('; example\nTEST 54 320 P 0.5 1.1 TEST\n0.1 100\n1.1 100\n1.2 0\nTEST2 75 440 P 1 2 TEST\n0 0\n1 100\n2 0');expect(m).toHaveLength(2);expect(m[0].curve[0]).toEqual({t:0,thrust:0});expect(m[0].totalMass).toBe(1.1);expect(motorStats(m[1]).impulse).toBe(100)});
 it('rejects malformed, negative and unfinished curves',()=>{for(const points of ['0 0\n0 -10','0 0\n1 20','1 0\n.5 0'])expect(()=>parseEng(`BAD 54 320 P .5 1 TEST\n${points}`)).toThrow()});
});
it('builds an isolated solver request with explicit event defaults',()=>{const p=initialProject(),f=initialFlight();f.configurations[0].mounts.umotor={motorId:'sample-54',event:'launch',delay:0,offset:0};const request=createFlightRequest(p,f,f.cases[0]);expect(request.motors).toHaveLength(1);expect(request.configuration.recovery.chute.event).toBe('apogee');expect(request.assembly.length).toBeGreaterThan(0);request.parts[0].name='Changed';expect(p.parts[0].name).not.toBe('Changed')});
it('rejects assignments to a part that is not a motor mount',()=>{const p=initialProject();p.flight=initialFlight();p.flight.configurations[0].mounts.nose={motorId:'sample-54',event:'launch',delay:0};expect(()=>validate(p)).toThrow()});
