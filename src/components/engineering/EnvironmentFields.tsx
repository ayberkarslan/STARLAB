import {useState} from 'react';
import {TextField} from '@stardrop/ui';
import {validateEnvironment,type Environment} from '../../domain/flight';
import {atmosphereColumns,windColumns,type EnvironmentInputs,type TurbulenceInputs} from '../../domain/environmentInputs';
import NumericSeriesEditor from './NumericSeriesEditor';
import {NumberInput} from '../NumberInput';
import {Choice,NumberField} from './shared';

export default function EnvironmentFields({value:e,onChange}:{value:Environment;onChange:(value:Environment)=>void}){
 const [error,setError]=useState('');
 function set(patch:Partial<Environment>){try{const next=validateEnvironment({...e,...patch});onChange(next);setError('')}catch(error){setError(error instanceof Error?error.message:'Çevre girdisi geçersiz.')}}
 const optional=(key:'windReferenceHeight'|'windMinimumHeight'|'windShearExponent'|'windRoughnessLength'|'windDirectionGradient',label:string,unit:string,signed=false)=><NumberInput label={label} key={key} unit={unit} optional signed={signed} min={key==='windDirectionGradient'||key==='windShearExponent'?0:Number.MIN_VALUE} value={e[key]} onChange={n=>set({[key]:n})}/>;
 const turbulence=(key:Exclude<keyof TurbulenceInputs,'enabled'>,label:string,unit:string,min=0,max=Infinity)=><NumberInput label={label} key={key} unit={unit} optional min={min} max={max} integer={key==='poles'||key==='seed'} value={e.turbulence?.[key]} onChange={n=>set({turbulence:{...e.turbulence,enabled:e.turbulence?.enabled??false,[key]:n}})}/>;
 return <div className="input-settings">
  <div className="engineering-field-grid"><Choice label="Atmosfer" value={e.atmosphereModel??'unknown'} options={[{value:'unknown',label:'Belirtilmedi'},{value:'standard1976',label:'Standart · US 1976'},{value:'surface',label:'Yüzey ölçümü'},{value:'profile',label:'İrtifa profili'}]} onChange={v=>set({atmosphereModel:v==='unknown'?undefined:v as EnvironmentInputs['atmosphereModel']})}/>
   <TextField label="Tarih · UTC" aria-label="Fırlatma tarihi UTC" type="datetime-local" value={e.dateTimeUtc?.slice(0,16)??''} onChange={event=>set({dateTimeUtc:event.target.value?event.target.value+'Z':undefined})}/>
  </div>
  {e.atmosphereModel==='profile'?<details className="engineering-details" open><summary>Atmosfer profili</summary><NumericSeriesEditor label="Atmosfer profili" columns={atmosphereColumns} maxRows={1000} value={e.atmosphereProfile?.map(p=>({...p}))} onApply={rows=>{const next=validateEnvironment({...e,atmosphereProfile:rows.map(r=>({altitude:r.altitude!,temperature:r.temperature!,pressure:r.pressure!,density:r.density,speedOfSound:r.speedOfSound,dynamicViscosity:r.dynamicViscosity}))});onChange(next)}}/></details>:e.atmosphereModel!=='standard1976'&&<div className="engineering-field-grid environment-surface"><NumberField label="Yüzey sıcaklığı" unit="°C" min={-100} max={100} value={e.temperature} onChange={temperature=>set({temperature})}/><NumberField label="Yüzey basıncı" unit="Pa" min={1} max={200000} value={e.pressure} onChange={pressure=>set({pressure})}/></div>}
  <details className="engineering-details" open><summary>Rüzgâr</summary>
   <Choice label="Rüzgâr modeli" value={e.windModel??'constant'} options={[{value:'constant',label:'Sabit'},{value:'profile',label:'İrtifa profili'},{value:'powerLaw',label:'Üs yasası'},{value:'logarithmic',label:'Logaritmik'}]} onChange={v=>set({windModel:v as EnvironmentInputs['windModel']})}/>
   {e.windModel==='profile'?<NumericSeriesEditor label="Rüzgâr profili" columns={windColumns} maxRows={1000} value={e.windProfile?.map(p=>({...p}))} onApply={rows=>{const next=validateEnvironment({...e,windProfile:rows.map(r=>({altitude:r.altitude!,speed:r.speed!,direction:r.direction!}))});onChange(next)}}/>:<div className="engineering-field-grid environment-surface"><NumberField label="Rüzgâr hızı" unit="m/s" max={200} value={e.windSpeed} onChange={windSpeed=>set({windSpeed})}/><NumberField label="Geldiği yön · kuzeyden" unit="°" max={360} value={e.windDirection} onChange={windDirection=>set({windDirection})}/>
    {['powerLaw','logarithmic'].includes(e.windModel??'')&&<>{optional('windReferenceHeight','Referans yüksekliği','m')}{optional('windMinimumHeight','Asgari yükseklik','m')}{e.windModel==='powerLaw'?optional('windShearExponent','Rüzgâr üssü','—'):optional('windRoughnessLength','Yüzey pürüzlülüğü','m')}{optional('windDirectionGradient','Yön değişimi','°/km',true)}</>}
   </div>}
  </details>
  <details className="engineering-details"><summary>Türbülans</summary><Choice label="Türbülans" value={e.turbulence?.enabled?'on':'off'} options={[{value:'off',label:'Kapalı'},{value:'on',label:'Açık'}]} onChange={v=>set({turbulence:{...e.turbulence,enabled:v==='on'}})}/>{e.turbulence?.enabled&&<div className="engineering-field-grid environment-surface">{turbulence('rms','RMS hız','m/s')}{turbulence('alpha','Spektrum katsayısı','—')}{turbulence('poles','Kutup sayısı','adet',1,100)}{turbulence('sampleRate','Örnekleme','Hz',Number.MIN_VALUE)}{turbulence('seed','Rastgelelik tohumu','—',0,4294967295)}</div>}</details>
  {error&&<p role="alert" className="series-error">{error}</p>}
 </div>;
}
