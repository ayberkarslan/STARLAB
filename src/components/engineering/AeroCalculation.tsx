import {useEffect,useRef,useState} from 'react';
import type {Project} from '../../domain/project';
import type {FlightWorkspace} from '../../domain/flight';
import {aeroInputs} from '../../engine/aeroInputs';
import {evaluateStructure} from '../../engine/client';
import type {EngineResponse} from '../../engine/protocol';
import {NumberInput} from '../NumberInput';
import {Choice,Metric,fmt} from './shared';
export default function AeroCalculation({project,flight}:{project:Project;flight:FlightWorkspace}){
 const [configurationId,setConfigurationId]=useState(''),[time,setTime]=useState(0),[revision,setRevision]=useState(0),[engineFailure,setEngineFailure]=useState('');
 const config=flight.configurations.find(c=>c.id===configurationId)??flight.configurations[0],hasMotor=project.parts.some(p=>p.kind==='motor');
 const projection=aeroInputs(project,flight,config?.id,time),key=JSON.stringify(projection),session=useRef(crypto.randomUUID());
 const [response,setResponse]=useState<{key:string;revision:number;result:EngineResponse}>(),[error,setError]=useState<{key:string;message:string}>();
 useEffect(()=>{const refresh=()=>{setEngineFailure('');setRevision(v=>v+1)},fail=(event:{message:string})=>{setEngineFailure(event.message);setResponse(undefined)};import.meta.hot?.on('starlab:engine-restarted',refresh);import.meta.hot?.on('starlab:engine-error',fail);return()=>{import.meta.hot?.off('starlab:engine-restarted',refresh);import.meta.hot?.off('starlab:engine-error',fail)}},[]);
 useEffect(()=>{const abort=new AbortController();setResponse(undefined);setError(undefined);if(projection.issues.length||engineFailure)return;
 const timer=setTimeout(()=>{void evaluateStructure({protocolVersion:1,requestId:crypto.randomUUID(),sessionId:session.current,model:'rocket-aero',inputs:projection.inputs,assembly:projection.assembly},abort.signal).then(result=>{if(!abort.signal.aborted)setResponse({key,revision,result})}).catch(e=>{if(!abort.signal.aborted)setError({key,message:e instanceof Error?e.message:'Hesap tamamlanamadı.'})})},120);
 return()=>{clearTimeout(timer);abort.abort()};},[key,revision,engineFailure]);
 const result=!engineFailure&&response?.key===key&&response.revision===revision?response.result:undefined;
 const fields=[['dynamicPressure','Dinamik basınç','Pa'],['axialForce','Eksenel aero kuvveti','N'],['normalForce','Normal aero kuvveti','N'],['stabilityMargin','Statik marj','cal'],['pitchMoment','Aero moment','N·m'],['dampingMoment','Sönüm momenti','N·m'],['totalMoment','Toplam yunuslama momenti','N·m']];
 const failed=result&&fields.some(([id])=>result.snapshot.results['aero_'+id]?.state.status!=='ready');
 return <div className="engineering-body">{hasMotor&&<div className="engineering-field-grid"><Choice label="Aero konfigürasyonu" value={config?.id??''} options={flight.configurations.map(c=>({value:c.id,label:c.name}))} onChange={setConfigurationId}/><NumberInput label="Aero değerlendirme zamanı" unit="s" value={time} onChange={v=>setTime(v??0)}/></div>}
 <div className="engineering-summary-grid" aria-label="Hesaplanan aero kuvvetleri">{fields.map(([id,label,unit])=>{const state=result?.snapshot.results['aero_'+id]?.state;return <Metric key={id} label={label} unit={unit} value={!failed&&state?.status==='ready'?fmt(state.value as number,6):'—'}/>})}</div>
 {projection.issues.length?<div className="engineering-readiness" role="status">{projection.issues.map((s,i)=><span key={i}>{s}</span>)}</div>:engineFailure?<p role="alert">{engineFailure}</p>:error?.key===key?<p role="alert">{error.message}</p>:failed?<p role="alert">Aero girdileri geçersiz; sonuçlar gösterilemiyor.</p>:!result?<p role="status">Hesaplanıyor…</p>:null}
 <details className="engineering-details"><summary>Model kapsamı</summary><p>Tek akış noktası; girilen katsayılar tüm rokete ve aynı referans alanına aittir. CP roket başlangıcından ölçülür, CG yapı hesabından gelir. Gövde kaldırmasını kapatmak için planform alanına 0; statik değerlendirme için yunuslama hızına 0 gir. Eksenel Cd ileri gövde eksenine aittir. Geometriden katsayı üretimi ve uçuş yörüngesi henüz bağlı değil.</p></details>
 </div>;
}
