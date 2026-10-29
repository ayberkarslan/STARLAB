/** Open-canopy inputs, separate from a packed component's aerodynamic override. */
export interface RecoveryInputs {
 role?:'main'|'drogue';
 dragCoefficient?:number;
 inflationTime?:number;
 referenceArea?:number;
 recoveredMass?:number;
 targetVelocity?:number;
 maxDeploymentPressure?:number;
}
export function validateRecoveryInputs(value:RecoveryInputs){
 if(value.role!==undefined&&!['main','drogue'].includes(value.role))throw Error('Paraşüt görevi geçersiz.');
 for(const key of ['dragCoefficient','inflationTime','referenceArea','recoveredMass','targetVelocity','maxDeploymentPressure'] as const){
  const n=value[key];if(n!==undefined&&(typeof n!=='number'||!Number.isFinite(n)||(key==='inflationTime'?n<0:n<=0)))throw Error('Açık paraşüt girdisi geçersiz.');
 }
}
