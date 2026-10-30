import {it,expect} from 'vitest';
import {defaultEnvironment,validateEnvironment} from '../domain/flight';
import {environmentInputs} from './environmentInputs';
const point={height:1000,groundDownrange:-5,groundVertical:100,referenceLength:1200};
it('keeps AGL and launch ASL separate and serializes Kelvin, Pascal and metre inputs',()=>{
 const r=environmentInputs({...defaultEnvironment,altitude:1500,temperature:20,atmosphereModel:'surface'},point);
 expect(r.issues).toEqual([]);expect(r.inputs).toMatchObject({atmosphereModel:0,height:1000,launchAltitude:1500,surfaceTemperature:293.15,surfacePressure:101325,referenceLength:1.2,groundDownrange:-5});
 expect(environmentInputs({...defaultEnvironment,atmosphereModel:'standard1976'},point).inputs.atmosphereModel).toBe(1);
});
it('preserves missing active shear inputs and admits source-supported zero minimum height',()=>{
 const e={...defaultEnvironment,atmosphereModel:'surface' as const,windModel:'powerLaw' as const,windReferenceHeight:10,windMinimumHeight:0,windShearExponent:0};
 expect(validateEnvironment(e)).toEqual(e);expect(environmentInputs(e,point).inputs).toMatchObject({windModel:1,minimumHeight:0,exponent:0});
 const missing=environmentInputs({...e,windReferenceHeight:undefined,windMinimumHeight:undefined,windShearExponent:undefined},point);
 expect(missing.inputs.referenceHeight).toBeUndefined();expect(missing.inputs.minimumHeight).toBeUndefined();expect(missing.inputs.exponent).toBeUndefined();
});
it('explicitly refuses profiles and turbulence instead of silently calculating constant wind',()=>{
 expect(environmentInputs(defaultEnvironment,point).issues.join()).toContain('Atmosfer modeli');
 expect(environmentInputs({...defaultEnvironment,atmosphereModel:'profile',windModel:'profile',turbulence:{enabled:true}},point).issues).toHaveLength(3);
 const r=environmentInputs({...defaultEnvironment,atmosphereModel:'surface'}, {...point,referenceLength:undefined});expect(r.inputs.referenceLength).toBeUndefined();
});
