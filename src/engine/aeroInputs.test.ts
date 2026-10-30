import {it,expect} from 'vitest';
import {calculationExample} from '../domain/calculationExample';
import {initialFlight} from '../domain/flight';
import {parseStar,serializeStar} from '../domain/starFile';
import {createProjectInputs} from './projectInputs';
import {aeroInputs} from './aeroInputs';
import {validateAerodynamics} from '../domain/aerodynamics';
const point={referenceDiameter:60,referenceArea:.003,mach:.3,angleOfAttack:5,normalForceSlope:8,dragCoefficient:.5,centerOfPressure:900,bodyPlanformArea:0,pitchRate:0,velocity:100,density:1.2};
it('converts point inputs to SI without calculating forces or substituting CG',()=>{
 const project=calculationExample();project.aerodynamics=point;
 const r=aeroInputs(project,initialFlight());expect(r.issues).toEqual([]);expect(r.inputs.cp).toBe(.9);expect(r.inputs.diameter).toBe(.06);expect(r.inputs.alpha).toBeCloseTo(Math.PI/36);expect(r.inputs.bodyMoment).toBe(0);
 expect(r.inputs).not.toHaveProperty('cg');expect(r.assembly).toHaveLength(3);
});
it('keeps missing coefficients unknown, accepts explicit zero, and requires body moment only for nonzero area',()=>{
 const project=calculationExample();project.aerodynamics={...point,normalForceSlope:undefined,pitchRate:undefined,bodyPlanformArea:undefined};
 expect(aeroInputs(project).issues.join()).toMatch(/CNa/);expect(aeroInputs(project).inputs.bodyMoment).toBeUndefined();
 project.aerodynamics={...point,normalForceSlope:0,centerOfPressure:0};expect(aeroInputs(project).issues).toEqual([]);
 project.aerodynamics.bodyPlanformArea=.1;expect(aeroInputs(project).issues.join()).toContain('ilk momenti');
 project.aerodynamics={...point,density:undefined,dynamicPressure:6000};expect(aeroInputs(project).issues).toEqual([]);expect(aeroInputs(project).inputs.pressure).toBe(6000);
});
it('round trips the whole-vehicle coefficients and preserves units in the input packet',()=>{
 const p=calculationExample();p.aerodynamics={...point,normalForceSlope:-1,centerOfPressure:-20,bodyPlanformMoment:-.02,pitchRate:-3};
 expect(parseStar(serializeStar(p)).aerodynamics).toEqual(p.aerodynamics);
 const packet=createProjectInputs(p);expect(packet.project.aerodynamics.centerOfPressure).toBe(-20);expect(packet.units.aerodynamics.pitchRate).toBe('deg/s');expect(packet.units.aerodynamics.bodyPlanformMoment).toBe('m³');
 expect(()=>validateAerodynamics({...point,dragCoefficient:-1})).toThrow();expect(()=>validateAerodynamics({...point,normalForceSlope:NaN})).toThrow();
});
