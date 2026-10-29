import {useState} from 'react';
import type {Part} from '../../domain/project';
import type {FlightConfiguration,Recovery} from '../../domain/flight';
import {validateRecoveryInputs} from '../../domain/recoveryInputs';
import {NumberInput} from '../NumberInput';
import {Choice,Empty,NumberField} from './shared';

export default function RecoveryFields({parts,configuration,onChange}:{parts:Part[];configuration:FlightConfiguration;onChange:(recovery:FlightConfiguration['recovery'])=>void}){
 const [error,setError]=useState('');
 return <div className="engineering-event-list input-settings">{parts.map(p=>{
  const value=configuration.recovery[p.id]??{event:'apogee' as const,delay:0,altitude:300};
  const set=(patch:Partial<Recovery>)=>{try{const next={...value,...patch};validateRecoveryInputs(next);onChange({...configuration.recovery,[p.id]:next});setError('')}catch(e){setError(e instanceof Error?e.message:'Kurtarma girdisi geçersiz.')}};
  const optional=(key:'dragCoefficient'|'inflationTime'|'referenceArea'|'recoveredMass'|'targetVelocity'|'maxDeploymentPressure',label:string,unit:string)=><NumberInput key={key} label={label} unit={unit} optional min={key==='inflationTime'?0:Number.MIN_VALUE} value={value[key]} onChange={n=>set({[key]:n})}/>;
  return <div key={p.id} className="engineering-event" role="group" aria-label={`${p.name} kurtarma`}><strong>{p.name}</strong>
   <div className="engineering-field-grid"><Choice label="Görev" value={value.role??'unknown'} options={[{value:'unknown',label:'Belirtilmedi'},{value:'main',label:'Ana paraşüt'},{value:'drogue',label:'Sürükleme paraşütü'}]} onChange={v=>set({role:v==='unknown'?undefined:v as Recovery['role']})}/>
    <Choice label="Açılma" value={value.event} options={[{value:'apogee',label:'Tepe noktası'},{value:'altitude',label:'İnişte irtifa'},{value:'never',label:'Devre dışı'}]} onChange={v=>set({event:v as Recovery['event']})}/>
    {value.event!=='never'&&<><NumberField label="Açılma gecikmesi" unit="s" value={value.delay} onChange={delay=>set({delay})}/>{value.event==='altitude'&&<NumberField label="Açılma irtifası · AGL" unit="m" value={value.altitude} onChange={altitude=>set({altitude})}/>}{optional('dragCoefficient','Açık paraşüt · Cd','—')}{optional('inflationTime','Şişme süresi','s')}</>}
   </div>
   {value.event!=='never'&&<details className="engineering-details"><summary>Boyutlandırma ve sınırlar</summary><div className="engineering-field-grid">{optional('referenceArea','Açık referans alanı','m²')}{optional('recoveredMass','Kurtarılan kütle','kg')}{optional('targetVelocity','Hedef iniş hızı','m/s')}{optional('maxDeploymentPressure','Azami açılma basıncı','Pa')}</div></details>}
  </div>;
 })}{!parts.length&&<Empty>Paraşüt eklenmedi</Empty>}{error&&<p role="alert" className="series-error">{error}</p>}</div>;
}
