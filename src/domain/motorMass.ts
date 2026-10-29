import type {Motor} from './motors';
import type {NumericColumn} from './numericSeries';
/** +X is the motor axis, CG is measured from its leading end in mm.
 * Principal-axis inertia components are about the local CG, in kg m².
 */
export interface MotorMassSample {t:number;totalMass:number;propellantMass:number;centerOfMass?:number;inertiaXX?:number;inertiaYY?:number;inertiaZZ?:number}
export const motorMassColumns:readonly NumericColumn[]=[
 {key:'t',label:'Zaman',unit:'s',min:0},
 {key:'totalMass',label:'Toplam kütle',unit:'kg',min:Number.MIN_VALUE},
 {key:'propellantMass',label:'Yakıt kütlesi',unit:'kg',min:0},
 {key:'centerOfMass',label:'CG',unit:'mm',optional:true,min:0},
 {key:'inertiaXX',label:'Ixx · eksenel',unit:'kg·m²',optional:true,min:0},
 {key:'inertiaYY',label:'Iyy',unit:'kg·m²',optional:true,min:0},
 {key:'inertiaZZ',label:'Izz',unit:'kg·m²',optional:true,min:0},
];
export function validateMotorMassProfile(value:unknown,motor:Pick<Motor,'totalMass'|'propellantMass'|'length'>):asserts value is MotorMassSample[]{
 if(!Array.isArray(value)||value.length<2||value.length>10000)throw Error('Motor kütle profilinde 2–10000 satır gerekli.');
 let last=-1;
 for(const p of value){
  if(!p||![p.t,p.totalMass,p.propellantMass].every(Number.isFinite)||p.t<0||p.t<=last||p.totalMass<=0||p.propellantMass<0||p.propellantMass>p.totalMass||p.totalMass>motor.totalMass+1e-6||p.propellantMass>motor.propellantMass+1e-6)throw Error('Motor kütle profili geçersiz; zaman artmalı ve kütleler motor sınırlarını aşmamalı.');
  for(const key of ['centerOfMass','inertiaXX','inertiaYY','inertiaZZ'] as const){const n=p[key];if(n!==undefined&&(typeof n!=='number'||!Number.isFinite(n)||n<0))throw Error('Motor CG / atalet girdisi geçersiz.');}
  if(p.centerOfMass!==undefined&&p.centerOfMass>motor.length)throw Error('Motor CG değeri motorun içinde olmalı.');
  last=p.t;
 }
 if(value[0].t!==0||Math.abs(value[0].totalMass-motor.totalMass)>1e-6||Math.abs(value[0].propellantMass-motor.propellantMass)>1e-6)throw Error('Profil t=0 ile başlamalı; ilk kütleler motor tanımıyla eşleşmeli.');
}
