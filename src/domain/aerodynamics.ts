/** Inputs for a static aerodynamic evaluation. Values are never flight results.
 * Lengths use the design frame (+X aft); angles are degrees, other units are SI.
 * An absent field is unknown. The process adapter owns derived quantities.
 */
export interface AerodynamicInputs {
 normalForceSlope?:number;
 dragCoefficient?:number;
 centerOfPressure?:number;
 bodyPlanformArea?:number;
 bodyPlanformMoment?:number;
 pitchRate?:number;
 referenceDiameter?:number;
 referenceArea?:number;
 referenceLength?:number;
 mach?:number;
 angleOfAttack?:number;
 reynolds?:number;
 velocity?:number;
 density?:number;
 dynamicViscosity?:number;
 dynamicPressure?:number;
 rollRate?:number;
 motorOn?:boolean;
}
export const aeroUnits={normalForceSlope:'1/rad',dragCoefficient:'1',centerOfPressure:'mm',bodyPlanformArea:'m²',bodyPlanformMoment:'m³',pitchRate:'deg/s',referenceDiameter:'mm',referenceArea:'m²',referenceLength:'mm',mach:'1',angleOfAttack:'deg',reynolds:'1',velocity:'m/s',density:'kg/m³',dynamicViscosity:'Pa·s',dynamicPressure:'Pa',rollRate:'deg/s'} as const;
export function validateAerodynamics(value:unknown):asserts value is AerodynamicInputs {
 if(!value||typeof value!=='object'||Array.isArray(value))throw Error('Aero koşulları geçersiz.');
 const v=value as AerodynamicInputs;
 for(const key of Object.keys(aeroUnits) as (keyof typeof aeroUnits)[]){
  const n=v[key];if(n===undefined)continue;
  if(typeof n!=='number'||!Number.isFinite(n)||!['angleOfAttack','rollRate','normalForceSlope','centerOfPressure','bodyPlanformMoment','pitchRate'].includes(key)&&n<0)throw Error('Aero girdisi geçersiz.');
  if(['referenceDiameter','referenceArea','referenceLength','density','dynamicViscosity'].includes(key)&&n<=0)throw Error('Referans ölçüleri ve akışkan özellikleri pozitif olmalı.');
 }
 if(v.angleOfAttack!==undefined&&Math.abs(v.angleOfAttack)>180)throw Error('Hücum açısı −180 ile 180 arasında olmalı.');
 if(v.motorOn!==undefined&&typeof v.motorOn!=='boolean')throw Error('Motor durumu geçersiz.');
}
export function aerodynamicInput(value:AerodynamicInputs={}){
 return {...Object.fromEntries(Object.keys(aeroUnits).map(key=>[key,value[key as keyof typeof aeroUnits]??null])) as Record<keyof typeof aeroUnits,number|null>,motorOn:value.motorOn??null};
}
