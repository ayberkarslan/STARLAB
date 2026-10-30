import {it,expect} from 'vitest';
import {motorInputs} from './motorInputs';
import {sampleMotors} from '../domain/motors';
it('transports source samples and SI masses without interpolating or adding points',()=>{
 const m=sampleMotors[0],r=motorInputs(m,.5);
 expect(r).toEqual({curve:m.curve.flatMap(p=>[p.t,p.thrust]),time:.5,wet:m.totalMass,propellant:m.propellantMass});
 expect(()=>motorInputs(m,-1)).toThrow();expect(()=>motorInputs({...m,propellantMass:m.totalMass+1},0)).toThrow();
});
