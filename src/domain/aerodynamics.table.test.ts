import {expect,it} from 'vitest';
import {validateAerodynamics,type AeroCoefficientRow} from './aerodynamics';
import {initialProject} from './project';
import {parseStar,serializeStar} from './starFile';

const row=(mach:number,reynolds:number):AeroCoefficientRow=>({Mach:mach,Reynolds:reynolds,AoA_deg:0,CA_off:.4,CA_on:.5,CN:0,CP_m:.3,CN_alpha_per_rad:2});
it('preserves a complete B3 aerodynamic grid in a project file',()=>{
 const project=initialProject();project.aerodynamics={coefficientTable:[row(.3,100000),row(.3,200000),row(1,100000),row(1,200000)]};
 validateAerodynamics(project.aerodynamics);
 expect(parseStar(serializeStar(project)).aerodynamics?.coefficientTable).toEqual(project.aerodynamics.coefficientTable);
});
it('rejects missing and duplicate aerodynamic grid points',()=>{
 expect(()=>validateAerodynamics({coefficientTable:[row(.3,100000),row(.3,200000),row(1,100000)]})).toThrow(/ızgarası eksik/);
 expect(()=>validateAerodynamics({coefficientTable:[row(.3,100000),row(.3,100000)]})).toThrow(/yinelenen/);
});
