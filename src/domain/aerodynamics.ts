/** Inputs for a static aerodynamic evaluation. Values are never flight results.
 * Lengths use the design frame (+X aft); angles are degrees, other units are SI.
 * An absent field is unknown. The process adapter owns derived quantities.
 */
export interface AerodynamicInputs {
 coefficientTable?:AeroCoefficientRow[];
 normalForceSlope?:number;
 dragCoefficient?:number;
 centerOfPressure?:number;
 surfaceRoughness?:number;
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
export interface AeroCoefficientRow {Mach:number;Reynolds:number;AoA_deg:number;CA_off:number;CA_on:number;CN:number;CP_m:number;CN_alpha_per_rad:number}
export function validateCoefficientTable(rows:AeroCoefficientRow[]){
 if(!Array.isArray(rows)||rows.length>10000)throw Error('Aero katsayı tablosu en fazla 10000 satır olabilir.');
 const axes=[new Set<number>(),new Set<number>(),new Set<number>()],keys=new Set<string>();
 for(const row of rows){
  if(!row||!['Mach','Reynolds','AoA_deg','CA_off','CA_on','CN','CP_m','CN_alpha_per_rad'].every(key=>typeof row[key as keyof AeroCoefficientRow]==='number'&&Number.isFinite(row[key as keyof AeroCoefficientRow])))throw Error('Aero katsayı tablosunda geçersiz sayı var.');
  if(row.Mach<0||row.Reynolds<0||row.AoA_deg<0)throw Error('Mach, Reynolds ve hücum açısı negatif olamaz.');
  const key=`${row.Mach}/${row.Reynolds}/${row.AoA_deg}`;if(keys.has(key))throw Error('Aero katsayı tablosunda yinelenen akış noktası var.');keys.add(key);
  axes[0].add(row.Mach);axes[1].add(row.Reynolds);axes[2].add(row.AoA_deg);
 }
 if(rows.length&&rows.length!==axes[0].size*axes[1].size*axes[2].size)throw Error('Aero tablosunda Mach × Reynolds × hücum açısı ızgarası eksik.');
}
export const aeroUnits={normalForceSlope:'1/rad',dragCoefficient:'1',centerOfPressure:'mm',surfaceRoughness:'µm',bodyPlanformArea:'m²',bodyPlanformMoment:'m³',pitchRate:'deg/s',referenceDiameter:'mm',referenceArea:'m²',referenceLength:'mm',mach:'1',angleOfAttack:'deg',reynolds:'1',velocity:'m/s',density:'kg/m³',dynamicViscosity:'Pa·s',dynamicPressure:'Pa',rollRate:'deg/s'} as const;
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
 if(v.coefficientTable!==undefined)validateCoefficientTable(v.coefficientTable);
}
export function aerodynamicInput(value:AerodynamicInputs={}){
 return {...Object.fromEntries(Object.keys(aeroUnits).map(key=>[key,value[key as keyof typeof aeroUnits]??null])) as Record<keyof typeof aeroUnits,number|null>,motorOn:value.motorOn??null,coefficientTable:value.coefficientTable??null};
}
