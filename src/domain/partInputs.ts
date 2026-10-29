import {isShell,type Kind} from './componentCatalog';
/** User supplied data only. Missing numbers are unknown, never computed defaults.
 * Axial coordinates: mm from this component's local leading origin, positive aft.
 * CG/CP on assemblies use the assembly origin. CP belongs to its stated flow condition.
 */
export interface PartInputs {
 wallThickness?:number;
 density?:number;
 /** Effective shear modulus in Pa; unset is unknown, not zero. */
 shearModulus?:number;
 /** Normal-force coefficient slope per radian; supplied override, not a result. */
 normalForceSlope?:number;
 surfaceDensity?:number;
 linearDensity?:number;
 centerOfMass?:number;
 centerOfPressure?:number;
 dragCoefficient?:number;
 roughness?:number;
 mach?:number;
 angleOfAttack?:number;
 massProvided?:boolean;
 massScope?:'component'|'subtree';
 notes?:string;
}
export function validatePartInputs(value:unknown,diameter:number,kind:Kind):asserts value is PartInputs {
 if(!value||typeof value!=='object'||Array.isArray(value))throw Error('Bileşen girdileri geçersiz.');
 const inputs=value as PartInputs;
 for(const key of ['wallThickness','density','shearModulus','normalForceSlope','surfaceDensity','linearDensity','centerOfMass','centerOfPressure','dragCoefficient','roughness','mach','angleOfAttack'] as const){
  const n=inputs[key];if(n===undefined)continue;
  if(typeof n!=='number'||!Number.isFinite(n)||(!['centerOfMass','centerOfPressure','angleOfAttack','normalForceSlope'].includes(key)&&n<0))throw Error('Bileşen sayısal girdisi geçersiz.');
 }
 if(inputs.shearModulus!==undefined&&inputs.shearModulus<=0)throw Error('Kayma modülü sıfırdan büyük olmalı.');
 if(inputs.density!==undefined&&inputs.density<=0)throw Error('Yoğunluk sıfırdan büyük olmalı.');
 if([inputs.surfaceDensity,inputs.linearDensity].some(n=>n!==undefined&&n<=0))throw Error('Yoğunluk sıfırdan büyük olmalı.');
 if(inputs.wallThickness!==undefined&&isShell(kind)&&inputs.wallThickness>diameter/2)throw Error('Et kalınlığı dış yarıçapı aşamaz.');
 if(inputs.angleOfAttack!==undefined&&Math.abs(inputs.angleOfAttack)>180)throw Error('Hücum açısı −180 ile 180 arasında olmalı.');
 if(inputs.massProvided!==undefined&&typeof inputs.massProvided!=='boolean')throw Error('Kütle durumu geçersiz.');
 if(inputs.massScope!==undefined&&!['component','subtree'].includes(inputs.massScope))throw Error('Kütle kapsamı geçersiz.');
 if(inputs.notes!==undefined&&(typeof inputs.notes!=='string'||inputs.notes.length>10000))throw Error('Bileşen notu en fazla 10000 karakter olabilir.');
}
