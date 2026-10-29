import {describe,it,expect} from 'vitest';
import {initialProject,validate} from './project';
import {initialFlight,validateEnvironment,simulationSignature} from './flight';
import {validateMotor,type Motor} from './motors';
import {parseStar,serializeStar} from './starFile';
import {createProjectInputs} from '../engine/projectInputs';
import {createFlightRequest} from '../engine/flightSolver';
import {validateAerodynamics} from './aerodynamics';

const motor=():Motor=>({id:'measured',name:'Measured',manufacturer:'Test',source:'custom',diameter:60,length:300,totalMass:2,propellantMass:1,curve:[{t:0,thrust:0},{t:1,thrust:100},{t:2,thrust:0}],massProfile:[{t:0,totalMass:2,propellantMass:1,centerOfMass:0,inertiaXX:0},{t:2,totalMass:1,propellantMass:0,centerOfMass:120}]});
function workspace(){
 const p=initialProject(),f=p.flight=initialFlight();
 p.aerodynamics={referenceDiameter:140,referenceArea:.02,referenceLength:2200,mach:0,angleOfAttack:0,rollRate:-2,motorOn:false};
 f.motors=[motor()];f.configurations[0].mounts.umotor={motorId:'measured',event:'launch',delay:0};
 f.configurations[0].recovery.chute={event:'altitude',altitude:300,delay:0,role:'main',dragCoefficient:1.5,inflationTime:0,referenceArea:2,recoveredMass:5,targetVelocity:6,maxDeploymentPressure:500};
 Object.assign(f.cases[0].environment,{atmosphereModel:'profile',atmosphereProfile:[{altitude:0,temperature:15,pressure:101325},{altitude:1000,temperature:8,pressure:90000,density:1.1}],windModel:'profile',windProfile:[{altitude:0,speed:0,direction:359},{altitude:1000,speed:10,direction:1}],dateTimeUtc:'2026-10-29T12:30Z',turbulence:{enabled:true,rms:1,alpha:1,poles:5,sampleRate:20,seed:0}});
 return p;
}
describe('extended flight input contracts',()=>{
 it('round trips all groups and preserves explicit zero, false and unknowns',()=>{
  const p=workspace();expect(parseStar(serializeStar(p))).toEqual(p);
  const packet=createProjectInputs(p);expect(packet.project.aerodynamics.motorOn).toBe(false);expect(packet.project.aerodynamics.mach).toBe(0);expect(packet.project.aerodynamics.density).toBeNull();
  expect(packet.project.flight.motors[0].massProfile?.[0].centerOfMass).toBe(0);expect(packet.project.flight.motors[0].massProfile?.[1].inertiaXX).toBeUndefined();
  expect(packet.units.aerodynamics.referenceArea).toBe('m²');expect(packet.units.motors.centerOfMass).toBe('mm');
  expect(packet.conventions.environment.altitude).toBe('mean-sea-level');expect(packet.conventions.recovery.altitude).toBe('above-launch-site');
  const f=p.flight!,request=createFlightRequest(p,f,f.cases[0]);expect(request.environment).toEqual(f.cases[0].environment);expect(request.configuration.recovery.chute.inflationTime).toBe(0);expect(request.motors[0].massProfile).toEqual(f.motors[0].massProfile);
  request.environment.windProfile![0].direction=20;request.motors[0].massProfile![0].totalMass=100;expect(f.cases[0].environment.windProfile![0].direction).toBe(359);expect(f.motors[0].massProfile![0].totalMass).toBe(2);
 });
 it('keeps old projects readable without creating guessed aero or profile values',()=>{
  const p=initialProject();expect(validate(p)).toBe(p);const packet=createProjectInputs(p);expect(packet.project.aerodynamics.referenceDiameter).toBeNull();expect(packet.project.flight.cases[0].environment.atmosphereModel).toBeUndefined();
 });
 it('invalidates existing results when any new consumed input changes',()=>{
  const p=workspace(),f=p.flight!,c=f.configurations[0],e=f.cases[0].environment,old=simulationSignature(p,c,e,f);
  const changes=[(q:typeof p)=>{q.aerodynamics!.referenceDiameter=150},(q:typeof p)=>{q.flight!.cases[0].environment.windProfile![1].direction=20},(q:typeof p)=>{q.flight!.motors[0].massProfile![1].centerOfMass=130},(q:typeof p)=>{q.flight!.configurations[0].recovery.chute.inflationTime=1}];
  for(const change of changes){const q=structuredClone(p);change(q);expect(simulationSignature(q,q.flight!.configurations[0],q.flight!.cases[0].environment,q.flight!)).not.toBe(old);}
  expect(simulationSignature({...p,name:'renamed'},c,e,f)).toBe(old);
 });
 it('rejects malformed aero fields while allowing signed rates and explicit motor off',()=>{
  for(const v of [{referenceDiameter:0},{referenceArea:-1},{mach:'1'},{angleOfAttack:181},{motorOn:0},{density:NaN}])expect(()=>validateAerodynamics(v)).toThrow();
  expect(()=>validateAerodynamics({mach:0,angleOfAttack:-3,rollRate:-10,motorOn:false})).not.toThrow();
 });
 it('rejects inconsistent motor mass samples without inventing a profile',()=>{
  expect(()=>validateMotor(motor())).not.toThrow();
  for(const change of [(m:Motor)=>{m.massProfile![0].t=1},(m:Motor)=>{m.massProfile![1].t=0},(m:Motor)=>{m.massProfile![0].totalMass=1.5},(m:Motor)=>{m.massProfile![1].propellantMass=2},(m:Motor)=>{m.massProfile![1].centerOfMass=301},(m:Motor)=>{m.massProfile![1].inertiaYY=-1}]){const m=motor();change(m);expect(()=>validateMotor(m)).toThrow();}
  const m=motor();delete m.massProfile;expect(validateMotor(m).massProfile).toBeUndefined();
 });
 it('rejects unsorted profiles and invalid environment or recovery data',()=>{
  const p=workspace(),e=p.flight!.cases[0].environment;
  for(const patch of [{atmosphereModel:'invented'},{windProfile:[{altitude:1,speed:0,direction:0},{altitude:1,speed:5,direction:90}]},{atmosphereProfile:[{altitude:0,temperature:15,pressure:0}]},{turbulence:{enabled:true,seed:1.5}},{dateTimeUtc:'2026-01-01T12:00+03:00'},{dateTimeUtc:'2026-02-31T12:00Z'}])expect(()=>validateEnvironment({...e,...patch} as never)).toThrow();
  for(const patch of [{dragCoefficient:0},{inflationTime:-1},{role:'invented'},{targetVelocity:Infinity}]){const q=structuredClone(p);Object.assign(q.flight!.configurations[0].recovery.chute,patch);expect(()=>validate(q)).toThrow();}
 });
});
