import {useEffect,useRef,useState} from 'react';
import type {Project} from '../../domain/project';
import {rocketInputs} from '../../engine/rocketInputs';
import {evaluateStructure} from '../../engine/client';
import type {EngineResponse} from '../../engine/protocol';
import type {FlightWorkspace} from '../../domain/flight';
import {NumberInput} from '../NumberInput';
import {Choice} from './shared';
import {Metric,fmt} from './shared';
export default function RocketMass({project,flight}:{project:Project;flight?:FlightWorkspace}){
 const [configurationId,setConfigurationId]=useState(''),[time,setTime]=useState(0);
 const config=flight?.configurations.find(c=>c.id===configurationId)??flight?.configurations[0],hasMotor=project.parts.some(p=>p.kind==='motor');
 const projection=rocketInputs(project,flight,config?.id,time),key=JSON.stringify(projection),session=useRef(crypto.randomUUID());
 const [response,setResponse]=useState<{key:string;revision:number;result:EngineResponse}>(),[error,setError]=useState<{key:string;message:string}>(),[revision,setRevision]=useState(0),[engineFailure,setEngineFailure]=useState('');
 useEffect(()=>{const refresh=()=>{setEngineFailure('');setRevision(v=>v+1)},fail=(event:{message:string})=>{setEngineFailure(event.message);setResponse(undefined)};import.meta.hot?.on('starlab:engine-restarted',refresh);import.meta.hot?.on('starlab:engine-error',fail);return()=>{import.meta.hot?.off('starlab:engine-restarted',refresh);import.meta.hot?.off('starlab:engine-error',fail)}},[]);
 useEffect(()=>{const abort=new AbortController();setResponse(undefined);setError(undefined);if(projection.issues.length||engineFailure)return;
 void evaluateStructure({protocolVersion:1,requestId:crypto.randomUUID(),sessionId:session.current,model:'rocket',inputs:{},assembly:projection.assembly},abort.signal).then(result=>{if(!abort.signal.aborted)setResponse({key,revision,result})}).catch(e=>{if(!abort.signal.aborted)setError({key,message:e instanceof Error?e.message:'Hesap tamamlanamadı.'})});return()=>abort.abort();
 },[key,revision,engineFailure]);
 const result=!engineFailure&&response?.key===key&&response.revision===revision?response.result:undefined;
 const fields=[['total_mass','Toplam kütle','kg'],['total_cg','Kütle merkezi · CG','m'],['total_axial','Eksenel atalet · Ixx','kg·m²'],['total_transverse','Enine atalet · Iyy = Izz','kg·m²']];
 const failed=result&&fields.some(([id])=>result.snapshot.results[id]?.state.status!=='ready');
 return <div className="engineering-body">{hasMotor&&<div className="engineering-field-grid"><Choice label="Konfigürasyon" value={config?.id??''} options={flight?.configurations.map(c=>({value:c.id,label:c.name}))??[]} onChange={setConfigurationId}/><NumberInput label="Simülasyon zamanı" unit="s" value={time} onChange={value=>setTime(value??0)}/></div>}<div className="engineering-summary-grid" aria-label="Hesaplanan roket yapısı">{fields.map(([id,label,unit])=>{const state=result?.snapshot.results[id]?.state;return <Metric key={id} label={label} unit={unit} value={!failed&&state?.status==='ready'?fmt(state.value as number,6):'—'}/>})}</div>
 {projection.issues.length?<div className="engineering-readiness" role="status">{projection.issues.map((issue,index)=><span key={index}>{issue}</span>)}</div>:engineFailure?<p role="alert">{engineFailure}</p>:error?.key===key?<p role="alert">{error.message}</p>:failed?<p role="alert">Bir bileşenin girdileri geçersiz; toplam yapı hesabı tamamlanamadı.</p>:!result?<p role="status">Hesaplanıyor…</p>:<p>{projection.assembly.length} parça · {result.snapshot.updated.length} node çalıştı · CG roket başlangıcından, ataletler toplam CG etrafında.</p>}
 {hasMotor&&<details><summary>Motor modeli sınırları</summary><p>CG motorun orta noktasında kabul edilir. Eski üretim uçuş modeli motorun eksenel ataletini sıfır alır; bu sürüm aynı sınırlamayı korur. Bağımsız kütle profili ve ayrılma henüz bağlı değil.</p></details>}
 </div>;
}
