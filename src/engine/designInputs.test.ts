import {it,expect} from 'vitest';
import {calculationExample} from '../domain/calculationExample';
import {initialFlight} from '../domain/flight';
import {designInputs} from './designInputs';

it('projects active wet structure and Mach-dependent static aero separately',()=>{
 const project=calculationExample(),flight=initialFlight();project.aerodynamics={mach:.3,angleOfAttack:2,referenceDiameter:60};
 const request=designInputs(project,flight);
 expect(request.issues).toEqual([]);expect(request.assembly).toHaveLength(3);expect(request.aeroAssembly).toHaveLength(2);
 expect(request.aeroAssembly[1]).toMatchObject({model:'fins-static-aero',position:1,inputs:{mach:.3,alpha:2*Math.PI/180,referenceArea:Math.PI*.03*.03}});
});

it('maps component CP and CNa overrides without replacing computed mass',()=>{
 const project=calculationExample(),flight=initialFlight(),nose=project.parts.find(p=>p.kind==='nose')!;
 nose.inputs={...nose.inputs,normalForceSlope:3.5,centerOfPressure:120};
 const request=designInputs(project,flight),input=request.aeroAssembly[0].inputs;
 expect(input).toMatchObject({cnaOverrideEnabled:1,cnaOverride:3.5,cpOverrideEnabled:1,cpOverride:.12});
 expect(request.assembly[0].model).toBe('nose');
});

it('binds transition structure and Barrowman aero from the same geometry',()=>{
 const project=calculationExample(),flight=initialFlight();project.parts.push({id:'transition',parentId:'stage',kind:'transition',name:'Geçiş',length:100,diameter:60,offset:0,mass:0,material:'Test',inputs:{wallThickness:2,density:1200},geometry:{aftDiameter:40}});
 const request=designInputs(project,flight);
 expect(request.issues).toEqual([]);expect(request.assembly.at(-1)?.model).toBe('transition');expect(request.aeroAssembly.at(-1)).toMatchObject({model:'transition-static-aero',inputs:{foreRadius:.03,aftRadius:.02}});
});

it('aggregates attached stages at the active configuration launch state',()=>{
 const project=calculationExample(),flight=initialFlight();project.parts.push({id:'booster',parentId:'rocket',kind:'stage',name:'Booster',length:0,diameter:60,offset:0,mass:0,material:'Test'},{id:'booster-body',parentId:'booster',kind:'body',name:'Alt gövde',length:500,diameter:60,offset:0,mass:0,material:'Test',inputs:{wallThickness:2,density:1200}});
 flight.configurations[0].separations.booster={event:'burnout',delay:0};const request=designInputs(project,flight);
 expect(request.issues).toEqual([]);expect(request.assembly.some(p=>p.id==='part:booster-body')).toBe(true);
});
