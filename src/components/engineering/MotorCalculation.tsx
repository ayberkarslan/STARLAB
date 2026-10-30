import {useEffect,useRef,useState} from 'react';
import {RangeField} from '@stardrop/ui';
import type {Motor} from '../../domain/motors';
import {motorInputs} from '../../engine/motorInputs';
import {evaluateStructure} from '../../engine/client';
import type {EngineResponse} from '../../engine/protocol';
import {NumberInput} from '../NumberInput';
import {Metric,fmt} from './shared';
export default function MotorCalculation({motor}:{motor:Motor}){
 const [time,setTime]=useState(0),[revision,setRevision]=useState(0),[failure,setFailure]=useState(''),[response,setResponse]=useState<{key:string;result:EngineResponse}>(),[error,setError]=useState<{key:string;message:string}>();
 const session=useRef(crypto.randomUUID()),inputs=motorInputs(motor,time),key=JSON.stringify([inputs,revision]),profile=Boolean(motor.massProfile?.length);
 useEffect(()=>setTime(0),[motor.id]);
 useEffect(()=>{const refresh=()=>{setFailure('');setRevision(v=>v+1)},failed=(e:{message:string})=>{setFailure(e.message);setResponse(undefined)};import.meta.hot?.on('starlab:engine-restarted',refresh);import.meta.hot?.on('starlab:engine-error',failed);return()=>{import.meta.hot?.off('starlab:engine-restarted',refresh);import.meta.hot?.off('starlab:engine-error',failed)}},[]);
 useEffect(()=>{const abort=new AbortController();setError(undefined);if(profile||failure)return;
 const timer=setTimeout(()=>{void evaluateStructure({protocolVersion:1,sessionId:session.current,requestId:crypto.randomUUID(),model:'motor',inputs},abort.signal).then(result=>{if(!abort.signal.aborted)setResponse({key,result})}).catch(e=>{if(!abort.signal.aborted)setError({key,message:e instanceof Error?e.message:'Motor hesabı tamamlanamadı.'})})},120);
 return()=>{clearTimeout(timer);abort.abort()};
 },[key,profile,failure]);
 const result=!profile&&!failure&&response?.key===key?response.result:undefined;
 const fields=[['thrust','Anlık itki','N'],['totalImpulse','Toplam impuls','N·s'],['remaining','Kalan yakıt','kg'],['mass','Anlık motor kütlesi','kg']];
 const failed=result&&fields.some(([id])=>result.snapshot.results[id]?.state.status!=='ready');
 const message=profile?'Girilen kütle profili henüz bu native motor hesabına bağlı değil.':failure||((error?.key===key&&error.message)||'');
 return <div className="engineering-body"><div className="engineering-field-grid"><RangeField label="Motor zamanını seç" min={0} max={motor.curve.at(-1)!.t} step={.001} unit="s" value={Math.min(time,motor.curve.at(-1)!.t)} onChange={setTime}/><NumberInput label="Motor zamanı" unit="s" value={time} onChange={v=>setTime(v??0)}/></div>
 <div className="engineering-summary-grid" aria-label="Native motor sonuçları">{fields.map(([id,label,unit])=>{const state=result?.snapshot.results[id]?.state;return <Metric key={id} label={label} unit={unit} value={!failed&&state?.status==='ready'?fmt(state.value as number,6):'—'}/>})}</div>
 {message?<p role="alert">{message}</p>:failed?<p role="alert">Motor girdileriyle hesap tamamlanamadı.</p>:!result?<p role="status">Hesaplanıyor…</p>:null}
 <details className="engineering-details"><summary>Hesap modeli</summary><p>Eski üretim kodu: parçalı doğrusal itki ve impuls oranından yakıt tüketimi. Zaman motorun yerel zamanı; ateşleme gecikmesi, motor CG/ataleti ve uçuş bu hesaba dahil değil. Eğri uçlarında eski programın sınır davranışı korunur.</p>{result&&<span>{result.snapshot.updated.length} node çalıştı · native C++</span>}</details>
 </div>;
}
