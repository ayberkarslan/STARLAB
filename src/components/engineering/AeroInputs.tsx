import {SelectMenu} from '@stardrop/ui';
import {validateCoefficientTable,type AerodynamicInputs,type AeroCoefficientRow} from '../../domain/aerodynamics';
import {NumberInput} from '../NumberInput';
import NumericSeriesEditor from './NumericSeriesEditor';
import type {NumericColumn} from '../../domain/numericSeries';

const coefficientColumns:readonly NumericColumn[]=[
 {key:'Mach',label:'Mach',unit:'1',min:0},{key:'Reynolds',label:'Reynolds',unit:'1',min:0},{key:'AoA_deg',label:'AoA',unit:'°',min:0},
 {key:'CA_off',label:'CA off',unit:'1'},{key:'CA_on',label:'CA on',unit:'1'},
 {key:'CN',label:'CN',unit:'1'},{key:'CP_m',label:'CP',unit:'m'},{key:'CN_alpha_per_rad',label:'CNa',unit:'1/rad'},
];

export default function AeroInputs({value={},onChange}:{value?:AerodynamicInputs;onChange:(value:AerodynamicInputs)=>void}){
 const field=(key:Exclude<keyof AerodynamicInputs,'motorOn'|'coefficientTable'>,label:string,unit:string,min=0,max=Infinity,signed=false)=><NumberInput key={key} label={label} unit={unit} optional min={min} max={max} signed={signed} value={value[key]} onChange={n=>onChange({...value,[key]:n})}/>;
 return <div className="engineering-body input-settings">
  <div className="engineering-field-grid">
   {field('referenceDiameter','Referans çapı','mm',Number.MIN_VALUE)}
   {field('referenceArea','Referans alanı','m²',Number.MIN_VALUE)}
   {field('referenceLength','Referans uzunluğu','mm',Number.MIN_VALUE)}
  </div>
  <details className="engineering-details" open><summary>Roket katsayıları</summary><div className="engineering-field-grid">
   {field('normalForceSlope','CNa · tüm roket','1/rad',0,Infinity,true)}
   {field('dragCoefficient','Eksenel Cd · tüm roket','—')}
   {field('centerOfPressure','CP · roket başlangıcından','mm',0,Infinity,true)}
   {field('surfaceRoughness','Yüzey pürüzlülüğü','µm')}
  </div></details>
  <details className="engineering-details"><summary>Gövde kaldırması ve yunuslama</summary><div className="engineering-field-grid">
   {field('bodyPlanformArea','Gövde planform alanı','m²')}
   {field('bodyPlanformMoment','Planform ilk momenti','m³',0,Infinity,true)}
   {field('pitchRate','Yunuslama hızı','°/s',0,Infinity,true)}
  </div></details>
  <details className="engineering-details" open><summary>Akış koşulu</summary><div className="engineering-field-grid">
   {field('mach','Mach','—')}{field('angleOfAttack','Hücum açısı','°',-180,180,true)}
   <label className="engineering-choice"><span>Motor</span><SelectMenu label="Aero motor durumu" value={value.motorOn===undefined?'unknown':value.motorOn?'on':'off'} options={[{value:'unknown',label:'Belirtilmedi'},{value:'on',label:'Yanıyor'},{value:'off',label:'Sönük'}]} onChange={v=>onChange({...value,motorOn:v==='unknown'?undefined:v==='on'})}/></label>
  </div></details>
  <details className="engineering-details"><summary>İleri akış girdileri</summary><div className="engineering-field-grid">
   {field('velocity','Akış hızı','m/s')}{field('reynolds','Reynolds sayısı','—')}
   {field('density','Hava yoğunluğu','kg/m³',Number.MIN_VALUE)}{field('dynamicViscosity','Dinamik viskozite','Pa·s',Number.MIN_VALUE)}
   {field('dynamicPressure','Dinamik basınç','Pa')}{field('rollRate','Yuvarlanma hızı','°/s',0,Infinity,true)}
  </div></details>
  <details className="engineering-details"><summary>Mach katsayı tablosu</summary><NumericSeriesEditor label="Aero katsayıları" columns={coefficientColumns} maxRows={10000} value={value.coefficientTable?.map(row=>({...row}))} onApply={rows=>{const table=rows as unknown as AeroCoefficientRow[];validateCoefficientTable(table);onChange({...value,coefficientTable:table})}}/></details>
 </div>;
}
