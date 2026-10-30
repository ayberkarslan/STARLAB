import {useEffect,useRef,useState} from 'react';
import type {Project} from '../../domain/project';
import {rocketInputs} from '../../engine/rocketInputs';
import {evaluateStructure} from '../../engine/client';
import type {EngineResponse} from '../../engine/protocol';
import {Metric,fmt} from './shared';
export default function RocketMass({project}:{project:Project}){
 const projection=rocketInputs(project),key=JSON.stringify(projection),session=useRef(crypto.randomUUID());
 const [response,setResponse]=useState<{key:string;revision:number;result:EngineResponse}>(),[error,setError]=useState<{key:string;message:string}>(),[revision,setRevision]=useState(0),[engineFailure,setEngineFailure]=useState('');
 useEffect(()=>{const refresh=()=>{setEngineFailure('');setRevision(v=>v+1)},fail=(event:{message:string})=>{setEngineFailure(event.message);setResponse(undefined)};import.meta.hot?.on('starlab:engine-restarted',refresh);import.meta.hot?.on('starlab:engine-error',fail);return()=>{import.meta.hot?.off('starlab:engine-restarted',refresh);import.meta.hot?.off('starlab:engine-error',fail)}},[]);
 useEffect(()=>{const abort=new AbortController();setResponse(undefined);setError(undefined);if(projection.issues.length||engineFailure)return;
 void evaluateStructure({protocolVersion:1,requestId:crypto.randomUUID(),sessionId:session.current,model:'rocket',inputs:{},assembly:projection.assembly},abort.signal).then(result=>{if(!abort.signal.aborted)setResponse({key,revision,result})}).catch(e=>{if(!abort.signal.aborted)setError({key,message:e instanceof Error?e.message:'Hesap tamamlanamadı.'})});return()=>abort.abort();
 },[key,revision,engineFailure]);
 const result=!engineFailure&&response?.key===key&&response.revision===revision?response.result:undefined;
 const fields=[['total_mass','Toplam yapı kütlesi','kg'],['total_cg','Kütle merkezi · CG','m'],['total_axial','Eksenel atalet · Ixx','kg·m²'],['total_transverse','Enine atalet · Iyy = Izz','kg·m²']];
 const failed=result&&fields.some(([id])=>result.snapshot.results[id]?.state.status!=='ready');
 return <div className="engineering-body"><div className="engineering-summary-grid" aria-label="Hesaplanan roket yapısı">{fields.map(([id,label,unit])=>{const state=result?.snapshot.results[id]?.state;return <Metric key={id} label={label} unit={unit} value={!failed&&state?.status==='ready'?fmt(state.value as number,6):'—'}/>})}</div>
 {projection.issues.length?<div className="engineering-readiness" role="status">{projection.issues.map((issue,index)=><span key={index}>{issue}</span>)}</div>:engineFailure?<p role="alert">{engineFailure}</p>:error?.key===key?<p role="alert">{error.message}</p>:failed?<p role="alert">Bir bileşenin girdileri geçersiz; toplam yapı hesabı tamamlanamadı.</p>:!result?<p role="status">Hesaplanıyor…</p>:<p>{projection.assembly.length} parça · {result.snapshot.updated.length} node çalıştı · CG roket başlangıcından, ataletler toplam CG etrafında.</p>}
 </div>;
}
