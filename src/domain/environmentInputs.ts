import type {NumericColumn} from './numericSeries';
export interface AtmospherePoint {altitude:number;temperature:number;pressure:number;density?:number;speedOfSound?:number;dynamicViscosity?:number}
export interface WindPoint {altitude:number;speed:number;direction:number}
export interface TurbulenceInputs {enabled:boolean;rms?:number;alpha?:number;poles?:number;sampleRate?:number;seed?:number}
export interface EnvironmentInputs {
 atmosphereModel?:'standard1976'|'surface'|'profile';
 atmosphereProfile?:AtmospherePoint[];
 windModel?:'constant'|'profile'|'powerLaw'|'logarithmic';
 windProfile?:WindPoint[];
 windReferenceHeight?:number;
 windMinimumHeight?:number;
 windShearExponent?:number;
 windRoughnessLength?:number;
 windDirectionGradient?:number;
 turbulence?:TurbulenceInputs;
 dateTimeUtc?:string;
}
export const atmosphereColumns:readonly NumericColumn[]=[
 {key:'altitude',label:'İrtifa',unit:'m · ASL',min:-600,max:100000},
 {key:'temperature',label:'Sıcaklık',unit:'°C',min:-150,max:150},
 {key:'pressure',label:'Basınç',unit:'Pa',min:Number.MIN_VALUE},
 {key:'density',label:'Yoğunluk',unit:'kg/m³',min:Number.MIN_VALUE,optional:true},
 {key:'speedOfSound',label:'Ses hızı',unit:'m/s',min:Number.MIN_VALUE,optional:true},
 {key:'dynamicViscosity',label:'Viskozite',unit:'Pa·s',min:Number.MIN_VALUE,optional:true},
];
export const windColumns:readonly NumericColumn[]=[
 {key:'altitude',label:'İrtifa',unit:'m · ASL',min:-600,max:100000},
 {key:'speed',label:'Hız',unit:'m/s',min:0,max:200},
 {key:'direction',label:'Geldiği yön',unit:'° · kuzeyden',min:0,max:360},
];
function series(value:unknown,columns:readonly NumericColumn[]){
 if(!Array.isArray(value)||value.length>1000)throw Error('Çevre profilinde en fazla 1000 satır olabilir.');
 let previous=-Infinity;
 for(const row of value){
  if(!row||typeof row!=='object')throw Error('Çevre profili satırı geçersiz.');
  for(const c of columns){const n=row[c.key];if(n===undefined&&c.optional)continue;if(typeof n!=='number'||!Number.isFinite(n)||n<(c.min??-Infinity)||n>(c.max??Infinity))throw Error(`Çevre profili: ${c.label} geçersiz.`);}
  if(row.altitude<=previous)throw Error('Profil irtifaları sıralı ve benzersiz olmalı.');previous=row.altitude;
 }
}
export function validateEnvironmentInputs(v:EnvironmentInputs){
 if(v.atmosphereModel!==undefined&&!['standard1976','surface','profile'].includes(v.atmosphereModel))throw Error('Atmosfer modeli geçersiz.');
 if(v.windModel!==undefined&&!['constant','profile','powerLaw','logarithmic'].includes(v.windModel))throw Error('Rüzgâr modeli geçersiz.');
 if(v.atmosphereProfile!==undefined)series(v.atmosphereProfile,atmosphereColumns);
 if(v.windProfile!==undefined)series(v.windProfile,windColumns);
 for(const key of ['windReferenceHeight','windMinimumHeight','windShearExponent','windRoughnessLength','windDirectionGradient'] as const){const n=v[key];if(n!==undefined&&(typeof n!=='number'||!Number.isFinite(n)||key!=='windDirectionGradient'&&(['windShearExponent','windMinimumHeight'].includes(key)?n<0:n<=0)))throw Error('Rüzgâr profili parametresi geçersiz.');}
 if(v.windReferenceHeight!==undefined&&v.windMinimumHeight!==undefined&&v.windMinimumHeight>v.windReferenceHeight)throw Error('Asgari rüzgâr yüksekliği referans yüksekliğini aşamaz.');
 if(v.turbulence!==undefined){
  const t=v.turbulence;if(!t||typeof t.enabled!=='boolean')throw Error('Türbülans ayarı geçersiz.');
  for(const key of ['rms','alpha','poles','sampleRate','seed'] as const){const n=t[key];if(n!==undefined&&(typeof n!=='number'||!Number.isFinite(n)||n<0))throw Error('Türbülans girdisi geçersiz.');}
  if(t.poles!==undefined&&(!Number.isInteger(t.poles)||t.poles<1||t.poles>100)||t.sampleRate!==undefined&&t.sampleRate<=0||t.seed!==undefined&&(!Number.isInteger(t.seed)||t.seed>4294967295))throw Error('Türbülans örnekleme girdisi geçersiz.');
 }
 if(v.dateTimeUtc!==undefined&&(typeof v.dateTimeUtc!=='string'||!/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}(:\d{2}(\.\d{1,3})?)?Z$/.test(v.dateTimeUtc)||!Number.isFinite(Date.parse(v.dateTimeUtc))))throw Error('Tarih UTC biçiminde olmalı.');
 if(v.dateTimeUtc!==undefined&&new Date(v.dateTimeUtc).toISOString().slice(0,16)!==v.dateTimeUtc.slice(0,16))throw Error('Takvim tarihi geçersiz.');
}
