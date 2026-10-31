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
