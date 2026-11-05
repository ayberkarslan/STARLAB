import {useEffect,useMemo,useRef,useState} from 'react';
import {Button} from '@stardrop/ui';
import {Download} from '@stardrop/ui/icons';
import type {AerodynamicInputs} from '../../domain/aerodynamics';
import {computeFinFlutterDiagnostics,generateMachAeroTable,machAeroTableCsv,machAeroTableToRasaeroGrid,rasaeroTableCsv} from '../../domain/aeroSweep';
import type {Project} from '../../domain/project';
import type {FlightWorkspace} from '../../domain/flight';
import {aeroInputs} from '../../engine/aeroInputs';
import {evaluateStructure} from '../../engine/client';
import type {EngineResponse} from '../../engine/protocol';
import {NumberInput} from '../NumberInput';
import Plot from './Plot';
import {Choice,Metric,download,fmt} from './shared';
export default function AeroCalculation({project,flight,onAerodynamicsChange}:{project:Project;flight:FlightWorkspace;onAerodynamicsChange?:(value:AerodynamicInputs)=>void}){
 const [configurationId,setConfigurationId]=useState(''),[time,setTime]=useState(0),[revision,setRevision]=useState(0),[engineFailure,setEngineFailure]=useState(''),[sweepPlot,setSweepPlot]=useState<'cd'|'cn'|'cp'>('cd');
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
 const cgState=result?.snapshot.results['rocket_total_cg']?.state;
 const cgMm=cgState?.status==='ready'&&typeof cgState.value==='number'?cgState.value*1000:undefined;
 const aoaDeg=project.aerodynamics?.angleOfAttack??4;
 const machRows=useMemo(()=>generateMachAeroTable(project,cgMm,aoaDeg),[project,cgMm,aoaDeg]);
 const maxSimSpeed=useMemo(()=>{
  const speeds=flight.cases.flatMap(c=>c.result?.samples.map(s=>s.velocity)??[]);
  return speeds.length?Math.max(...speeds):undefined;
 },[flight.cases]);
 const flutterRows=useMemo(()=>computeFinFlutterDiagnostics(project,maxSimSpeed),[project,maxSimSpeed]);

 return <div className="engineering-body">{hasMotor&&<div className="engineering-field-grid"><Choice label="Aero konfigürasyonu" value={config?.id??''} options={flight.configurations.map(c=>({value:c.id,label:c.name}))} onChange={setConfigurationId}/><NumberInput label="Aero değerlendirme zamanı" unit="s" value={time} onChange={v=>setTime(v??0)}/></div>}
 <div className="engineering-summary-grid" aria-label="Hesaplanan aero kuvvetleri">{fields.map(([id,label,unit])=>{const state=result?.snapshot.results['aero_'+id]?.state;return <Metric key={id} label={label} unit={unit} value={!failed&&state?.status==='ready'?fmt(state.value as number,6):'—'}/>})}</div>
 {projection.issues.length?<div className="engineering-readiness" role="status">{projection.issues.map((s,i)=><span key={i}>{s}</span>)}</div>:engineFailure?<p role="alert">{engineFailure}</p>:error?.key===key?<p role="alert">{error.message}</p>:failed?<p role="alert">Aero girdileri geçersiz; sonuçlar gösterilemiyor.</p>:!result?<p role="status">Hesaplanıyor…</p>:null}
 <details className="engineering-details" open><summary>Mach 0,30 – 2,00 aerodinamik katsayı tablosu ve dışa aktarım (KTR)</summary>
  <div className="engineering-small-actions" style={{marginBottom:12}}>
   <Choice label="Mach grafiği" value={sweepPlot} options={[{value:'cd',label:'CD (Takatsiz / Takatli)'},{value:'cn',label:'CN ve Cm'},{value:'cp',label:'Basınç merkezi (CP)'}]} onChange={v=>setSweepPlot(v as 'cd'|'cn'|'cp')}/>
   <Button size="sm" variant="secondary" onClick={()=>download('starlab-aero-mach-0.3-2.0.csv',machAeroTableCsv(machRows),'text/csv')}><Download size={14}/>Mach 0,3–2,0 Tablosu (CSV)</Button>
   <Button size="sm" variant="secondary" onClick={()=>download('starlab-rasaero-cfd-tablosu.csv',rasaeroTableCsv(project.aerodynamics?.coefficientTable?.length?project.aerodynamics.coefficientTable:machAeroTableToRasaeroGrid(project,cgMm)),'text/csv')}><Download size={14}/>CFD / RASAero Tablosu (CSV)</Button>
   {onAerodynamicsChange&&<Button size="sm" variant="ghost" onClick={()=>onAerodynamicsChange({...(project.aerodynamics??{}),coefficientTable:machAeroTableToRasaeroGrid(project,cgMm)})}>Uçuş tablosuna uygula (Mach×Re×AoA)</Button>}
  </div>
  <div style={{height:210,marginBottom:12}}>
   <Plot title="Mach sayısına bağlı aerodinamik katsayı değişimi" xLabel="Mach" yLabel={sweepPlot==='cd'?'CD':sweepPlot==='cn'?'Katsayı':'CP · mm'} series={
    sweepPlot==='cd'?[
     {id:'cd-off',name:'CD (Takatsiz)',points:machRows.map(r=>[r.mach,r.cdOff])},
     {id:'cd-on',name:'CD (Takatli · İtki sönümlemeli)',points:machRows.map(r=>[r.mach,r.cdOn])},
    ]:sweepPlot==='cn'?[
     {id:'cn',name:`CN (α=${aoaDeg}°)`,points:machRows.map(r=>[r.mach,r.cn])},
     {id:'cm',name:`Cm (α=${aoaDeg}°)`,points:machRows.map(r=>[r.mach,r.cm])},
    ]:[
     {id:'cp',name:'CP konumu (mm)',points:machRows.map(r=>[r.mach,r.cpMm])},
    ]
   }/>
  </div>
  <div className="engineering-scroll" style={{maxHeight:240}}>
   <table className="engineering-table">
    <thead><tr><th>Mach</th><th>Rejim</th><th>CD (Takatsiz)</th><th>CD (Takatli)</th><th>CNa (1/rad)</th><th>CN</th><th>Cm</th><th>CP · mm</th><th>Marj · cal</th></tr></thead>
    <tbody>{machRows.map(r=><tr key={r.mach}><td>{fmt(r.mach,2)}</td><td>{r.regime}</td><td>{fmt(r.cdOff,4)}</td><td>{fmt(r.cdOn,4)}</td><td>{fmt(r.cnAlphaPerRad,3)}</td><td>{fmt(r.cn,4)}</td><td>{fmt(r.cm,4)}</td><td>{fmt(r.cpMm,1)}</td><td>{fmt(r.marginCal,2)}</td></tr>)}</tbody>
   </table>
  </div>
 </details>
 {flutterRows.length>0&&<details className="engineering-details" open><summary>Kanatçık çırpınması (Fin Flutter) ve normal şok analizi</summary>
  <table className="engineering-table">
   <thead><tr><th>Kanatçık</th><th>Kalınlık / Kök</th><th>Kayma modülü G</th><th>Flutter hızı</th><th>Flutter Mach</th><th>Normal şok Mach</th><th>Durum</th></tr></thead>
   <tbody>{flutterRows.map(f=><tr key={f.partId}><td>{f.name}</td><td>{fmt(f.thicknessMm,1)} / {fmt(f.rootChordMm,0)} mm</td><td>{fmt(f.shearModulusGpa,1)} GPa</td><td>{fmt(f.flutterSpeedMs,1)} m/s</td><td>M {fmt(f.flutterMach,2)}</td><td>M {fmt(f.normalShockCritMach,2)}</td><td><span className="engineering-badge">{f.status==='critical'?'Kritik flutter riski':f.status==='warning'?'Sınırda · kontrol et':f.safetyRatio?`Güvenli (FS ${fmt(f.safetyRatio,2)})`:'Güvenli'}</span></td></tr>)}</tbody>
  </table>
 </details>}
 <details className="engineering-details"><summary>Model kapsamı</summary><p>Tek akış noktası; girilen katsayılar tüm rokete ve aynı referans alanına aittir. CP roket başlangıcından ölçülür, CG yapı hesabından gelir. Gövde kaldırmasını kapatmak için planform alanına 0; statik değerlendirme için yunuslama hızına 0 gir. Eksenel Cd ileri gövde eksenine aittir. Geometriden katsayı üretimi ve uçuş yörüngesi henüz bağlı değil.</p></details>
 </div>;
}
