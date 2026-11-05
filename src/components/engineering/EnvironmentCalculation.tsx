import {useEffect,useMemo,useRef,useState} from 'react';
import {Button} from '@stardrop/ui';
import type {Environment} from '../../domain/flight';
import {environmentProfileCsv,generateEnvironmentProfile} from '../../domain/environmentProfile';
import {environmentInputs} from '../../engine/environmentInputs';
import {evaluateStructure} from '../../engine/client';
import type {EngineResponse} from '../../engine/protocol';
import {NumberInput} from '../NumberInput';
import Plot,{type PlotSeries} from './Plot';
import {Choice,Metric,fmt} from './shared';

type ProfileMetricKey='densityKgM3'|'pressurePa'|'temperatureC'|'speedOfSoundMs'|'windSpeedMs';
const PROFILE_METRICS:Record<ProfileMetricKey,{label:string;unit:string;digits:number}>={
 densityKgM3:{label:'Hava yoğunluğu',unit:'kg/m³',digits:4},
 pressurePa:{label:'Basınç',unit:'Pa',digits:0},
 temperatureC:{label:'Sıcaklık',unit:'°C',digits:2},
 speedOfSoundMs:{label:'Ses hızı',unit:'m/s',digits:1},
 windSpeedMs:{label:'Rüzgâr hızı',unit:'m/s',digits:2},
};

export default function EnvironmentCalculation({environment,referenceLength}:{environment:Environment;referenceLength?:number}){
 const [height,setHeight]=useState(0),[horizontal,setHorizontal]=useState(0),[vertical,setVertical]=useState(0),[length,setLength]=useState(referenceLength),[revision,setRevision]=useState(0),[engineFailure,setEngineFailure]=useState('');
 const [maxProfileAltM,setMaxProfileAltM]=useState(5000);
 const [profileMetric,setProfileMetric]=useState<ProfileMetricKey>('densityKgM3');
 const projection=environmentInputs(environment,{height,groundDownrange:horizontal,groundVertical:vertical,referenceLength:length}),key=JSON.stringify(projection),session=useRef(crypto.randomUUID());
 const [response,setResponse]=useState<{key:string;revision:number;result:EngineResponse}>(),[error,setError]=useState<{key:string;message:string}>();
 useEffect(()=>{const refresh=()=>{setEngineFailure('');setRevision(v=>v+1)},fail=(event:{message:string})=>{setEngineFailure(event.message);setResponse(undefined)};import.meta.hot?.on('starlab:engine-restarted',refresh);import.meta.hot?.on('starlab:engine-error',fail);return()=>{import.meta.hot?.off('starlab:engine-restarted',refresh);import.meta.hot?.off('starlab:engine-error',fail)}},[]);
 useEffect(()=>{const abort=new AbortController();setResponse(undefined);setError(undefined);if(projection.issues.length||engineFailure)return;
 const timer=setTimeout(()=>{void evaluateStructure({protocolVersion:1,requestId:crypto.randomUUID(),sessionId:session.current,model:'environment',inputs:projection.inputs},abort.signal).then(result=>{if(!abort.signal.aborted)setResponse({key,revision,result})}).catch(e=>{if(!abort.signal.aborted)setError({key,message:e instanceof Error?e.message:'Hesap tamamlanamadı.'})})},120);
 return()=>{clearTimeout(timer);abort.abort()};},[key,revision,engineFailure]);
 const result=!engineFailure&&response?.key===key&&response.revision===revision?response.result:undefined;
 const fields=[['temperature','Sıcaklık','K'],['pressure','Basınç','Pa'],['density','Hava yoğunluğu','kg/m³'],['speedOfSound','Ses hızı','m/s'],['viscosity','Viskozite','Pa·s'],['windSpeed','Yerel rüzgâr','m/s'],['windEast','Doğu rüzgârı','m/s'],['windNorth','Kuzey rüzgârı','m/s'],['windAlong','Düzlemdeki rüzgâr','m/s'],['speed','Havaya göre hız','m/s'],['mach','Mach','—'],['reynolds','Reynolds','—'],['dynamicPressure','Dinamik basınç','Pa'],['gravity','Yerçekimi','m/s²']];
 const blocked=result?[...result.manifest.nodes,...result.manifest.sources].flatMap(n=>{const state=result.snapshot.results[n.id]?.state;return state?.status==='error'?[`${n.label}: ${state.error}`]:state?.status==='missing'&&n.id!=='referenceLength'?[`${n.label}: veri gerekli.`]:[]}):[];

 const profileSamples=useMemo(()=>generateEnvironmentProfile(environment,maxProfileAltM,Math.max(50,Math.round(maxProfileAltM/20))),[environment,maxProfileAltM]);
 const profileMeta=PROFILE_METRICS[profileMetric];
 const profileSeries=useMemo<PlotSeries[]>(()=>[{
  id:profileMetric,
  name:`${profileMeta.label} (${profileMeta.unit})`,
  points:profileSamples.map(s=>[s.altitudeAglM,s[profileMetric]] as [number,number]),
 }],[profileSamples,profileMetric,profileMeta]);

 function downloadText(content:string,filename:string){
  const blob=new Blob([content],{type:'text/csv;charset=utf-8'});
  const url=URL.createObjectURL(blob);
  const a=document.createElement('a');
  a.href=url;a.download=filename;a.click();
  URL.revokeObjectURL(url);
 }

 return <div className="input-settings"> {projection.issues.length?<div className="engineering-readiness" role="status">{projection.issues.map(s=><span key={s}>{s}</span>)}</div>:engineFailure?<p role="alert">{engineFailure}</p>:error?.key===key?<p role="alert">{error.message}</p>:!result?<p role="status">Hesaplanıyor…</p>:blocked.length?<div role="alert">{blocked.map(s=><p key={s}>{s}</p>)}</div>:null}<div className="engineering-field-grid"><NumberInput label="İrtifa · fırlatma üstünden" unit="m" value={height} max={86000} onChange={v=>setHeight(v??0)}/><NumberInput label="Yere göre yatay hız" unit="m/s" value={horizontal} signed onChange={v=>setHorizontal(v??0)}/><NumberInput label="Yere göre dikey hız" unit="m/s" value={vertical} signed onChange={v=>setVertical(v??0)}/><NumberInput label="Reynolds referans uzunluğu" unit="mm" value={length} optional min={Number.MIN_VALUE} onChange={setLength}/></div>
 <div aria-label="Hesaplanan çevre"><div className="engineering-summary-grid environment-calculation-results">{fields.filter(([id])=>['temperature','pressure','density','windAlong','speed','mach'].includes(id)).map(([id,label,unit])=>{const state=result?.snapshot.results[id]?.state;return <Metric key={id} label={label} unit={unit} value={state?.status==='ready'?fmt(state.value as number,6):'—'}/>})}</div><details className="engineering-details"><summary>Akış ayrıntıları</summary><div className="engineering-summary-grid environment-calculation-results">{fields.filter(([id])=>!['temperature','pressure','density','windAlong','speed','mach'].includes(id)).map(([id,label,unit])=>{const state=result?.snapshot.results[id]?.state;return <Metric key={id} label={label} unit={unit} value={state?.status==='ready'?fmt(state.value as number,id==='viscosity'?9:6):'—'}/>})}</div></details></div>

 <details className="engineering-details" open>
  <summary>İrtifaya Bağlı Atmosfer ve Rüzgâr Profili (AR-003 / AR-007)</summary>
  <div className="engineering-field-grid" style={{marginBottom:8}}>
   <Choice label="Grafik değişkeni" value={profileMetric} options={(Object.keys(PROFILE_METRICS) as ProfileMetricKey[]).map(k=>({value:k,label:`${PROFILE_METRICS[k].label} (${PROFILE_METRICS[k].unit})`}))} onChange={v=>setProfileMetric(v as ProfileMetricKey)}/>
   <NumberInput label="Tavan irtifa (AGL)" unit="m" min={500} max={86000} value={maxProfileAltM} onChange={v=>setMaxProfileAltM(v??5000)}/>
  </div>
  <div className="toolbar-actions" style={{display:'flex',gap:8,flexWrap:'wrap',marginBottom:8}}>
   <Button variant="ghost" onClick={()=>downloadText(environmentProfileCsv(profileSamples),'atmosfer-ruzgar-profili.csv')}>Atmosfer & Rüzgâr Profilini İndir (CSV)</Button>
  </div>
  <div style={{height:210,marginBottom:10}}>
   <Plot title="İrtifaya bağlı atmosfer ve rüzgâr profili" xLabel="İrtifa AGL · m" yLabel={`${profileMeta.label} · ${profileMeta.unit}`} series={profileSeries}/>
  </div>
  <div style={{maxHeight:200,overflowY:'auto'}}>
   <table className="engineering-table">
    <thead>
     <tr>
      <th>AGL (m)</th>
      <th>ASL (m)</th>
      <th>Sıcaklık (°C)</th>
      <th>Basınç (Pa)</th>
      <th>Yoğunluk (kg/m³)</th>
      <th>Ses hızı (m/s)</th>
      <th>Rüzgâr (m/s)</th>
      <th>Yön (°)</th>
     </tr>
    </thead>
    <tbody>
     {profileSamples.map(s=>(
      <tr key={s.altitudeAglM}>
       <td>{fmt(s.altitudeAglM,0)}</td>
       <td>{fmt(s.altitudeAslM,0)}</td>
       <td>{fmt(s.temperatureC,1)}</td>
       <td>{fmt(s.pressurePa,0)}</td>
       <td>{fmt(s.densityKgM3,4)}</td>
       <td>{fmt(s.speedOfSoundMs,1)}</td>
       <td>{fmt(s.windSpeedMs,2)}</td>
       <td>{fmt(s.windDirectionDeg,0)}°</td>
      </tr>
     ))}
    </tbody>
   </table>
  </div>
 </details>

 <details className="engineering-details"><summary>Model kapsamı</summary><p>Tek çevre/akış noktası ve irtifaya bağlı atmosfer/rüzgâr profili özetidir. Yüzey ölçümü AGL, US 1976 ASL irtifasını kullanır. Desteklenen mutlak irtifa −600…86.000 m. Rüzgâr meteorolojik geldiği yöne göre, ray azimutunun uçuş düzlemine yansıtılır.</p></details></div>;
}
