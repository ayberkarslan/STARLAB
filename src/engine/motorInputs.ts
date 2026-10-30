import {validateMotor,type Motor} from '../domain/motors';
/** Serialization only: no interpolation, integration or mass calculation in the UI. */
export function motorInputs(motor:Motor,time:number){
 validateMotor(motor);
 if(!Number.isFinite(time)||time<0)throw Error('Motor zamanı negatif olmayan sonlu bir sayı olmalı.');
 return {curve:motor.curve.flatMap(p=>[p.t,p.thrust]),time,wet:motor.totalMass,propellant:motor.propellantMass};
}
