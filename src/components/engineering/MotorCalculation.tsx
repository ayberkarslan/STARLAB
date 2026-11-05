import {useEffect,useMemo,useRef,useState} from 'react';
import {Button,RangeField,TextField} from '@stardrop/ui';
import type {Motor} from '../../domain/motors';
import {
 PROPELLANT_PRESETS,
 computePreliminaryMotorDesign,
 generateMotorTimeSeries,
 motorTimeSeriesCsv,
 rankCatalogMotorsForPreliminaryDesign,
 type PreliminaryMotorDesignInputs,
 type PropellantPresetId,
 type ThrustProfileShape,
} from '../../domain/motorPreliminaryDesign';
import {motorInputs} from '../../engine/motorInputs';
import {evaluateStructure} from '../../engine/client';
import type {EngineResponse} from '../../engine/protocol';
import {NumberInput} from '../NumberInput';
import Plot,{type PlotSeries} from './Plot';
import {Choice,Metric,download,fmt} from './shared';

type SeriesView='thrust'|'mass'|'mdot'|'cg';

export default function MotorCalculation({motor,allCatalogMotors=[],onCreateMotor}:{motor:Motor;allCatalogMotors?:Motor[];onCreateMotor?:(motor:Motor)=>void}){
 const [time,setTime]=useState(0),[revision,setRevision]=useState(0),[failure,setFailure]=useState(''),[response,setResponse]=useState<{key:string;result:EngineResponse}>(),[error,setError]=useState<{key:string;message:string}>();
 const [seriesView,setSeriesView]=useState<SeriesView>('mass');
 const [designInputs,setDesignInputs]=useState<PreliminaryMotorDesignInputs>({
  name:'STAR-H25K',
  manufacturer:'STARLAB Ön Tasarım',
  presetId:'n2o_htpb_hybrid',
  targetImpulseNs:25000,
  burnTimeS:8.5,
  ispS:PROPELLANT_PRESETS.n2o_htpb_hybrid.defaultIspS,
  ofRatio:PROPELLANT_PRESETS.n2o_htpb_hybrid.defaultOfRatio,
  outerDiameterMm:150,
  chamberPressureBar:35,
  tankPressureBar:55,
  allowableStressMpa:276,
  safetyFactor:1.8,
  casingDensityKgM3:2700,
  expansionRatio:6.5,
  profileShape:'regressive',
 });

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
 const message=profile?'Girilen kütle profili zaman serisi üzerinden doğrudan değerlendiriliyor.':failure||((error?.key===key&&error.message)||'');

 const timeSeries=useMemo(()=>generateMotorTimeSeries(motor,50),[motor]);
 const avgIsp=timeSeries[0]?.ispS??0;
 const peakMdot=Math.max(...timeSeries.map(s=>s.massFlowRateKgS),0);

 const plotSeries=useMemo<PlotSeries[]>(()=>{
  if(seriesView==='thrust'){
   return [{id:'thrust',name:'İtki F(t) (N)',points:timeSeries.map(s=>[s.t,s.thrustN])}];
  }
  if(seriesView==='mdot'){
   return [{id:'mdot',name:'Kütle debisi ṁ(t) (kg/s)',points:timeSeries.map(s=>[s.t,s.massFlowRateKgS])}];
  }
  if(seriesView==='cg'){
   return [{id:'cg',name:'Motor CG konumu (mm · ön uçtan)',points:timeSeries.map(s=>[s.t,s.cgFromFrontMm])}];
  }
  return [
   {id:'totalMass',name:'Toplam motor kütlesi (kg)',points:timeSeries.map(s=>[s.t,s.totalMassKg])},
   {id:'propMass',name:'Kalan yakıt kütlesi (kg)',points:timeSeries.map(s=>[s.t,s.propellantMassKg])},
  ];
 },[timeSeries,seriesView]);

 const prelimResult=useMemo(()=>computePreliminaryMotorDesign(designInputs),[designInputs]);
 const catalogMatches=useMemo(()=>rankCatalogMotorsForPreliminaryDesign({
  impulseNs:designInputs.targetImpulseNs,
  burnTimeS:designInputs.burnTimeS,
  totalMassKg:prelimResult.totalMotorMassKg,
 },allCatalogMotors).slice(0,4),[designInputs.targetImpulseNs,designInputs.burnTimeS,prelimResult.totalMotorMassKg,allCatalogMotors]);

 function updatePreset(presetId:PropellantPresetId){
  const p=PROPELLANT_PRESETS[presetId];
  setDesignInputs(prev=>({...prev,presetId,ispS:p.defaultIspS,ofRatio:p.defaultOfRatio}));
 }

 return <div className="engineering-body">
  <div className="engineering-field-grid"><RangeField label="Motor zamanını seç" min={0} max={motor.curve.at(-1)!.t} step={.001} unit="s" value={Math.min(time,motor.curve.at(-1)!.t)} onChange={setTime}/><NumberInput label="Motor zamanı" unit="s" value={time} onChange={v=>setTime(v??0)}/></div>
  <div className="engineering-summary-grid" aria-label="Native motor sonuçları">
   {fields.map(([id,label,unit])=>{const state=result?.snapshot.results[id]?.state;return <Metric key={id} label={label} unit={unit} value={!failed&&state?.status==='ready'?fmt(state.value as number,4):'—'}/>})}
   <Metric label="Özgül itki (Isp)" unit="s" value={fmt(avgIsp,1)}/>
   <Metric label="Azami kütle debisi (ṁ)" unit="kg/s" value={fmt(peakMdot,3)}/>
  </div>
  {message?<p role="status">{message}</p>:failed?<p role="alert">Motor girdileriyle hesap tamamlanamadı.</p>:!result?<p role="status">Hesaplanıyor…</p>:null}

  <details className="engineering-details" open>
   <summary>Motor Zaman Serileri: F(t), m(t), ṁ(t), Isp ve CG(t) (Mİ-006 / Mİ-007)</summary>
   <div className="engineering-field-grid" style={{marginBottom:8}}>
    <Choice label="Grafik serisi" value={seriesView} options={[
     {value:'mass',label:'Kütle m(t) & Yakıt m_p(t) (kg)'},
     {value:'mdot',label:'Kütle debisi ṁ(t) (kg/s)'},
     {value:'cg',label:'Motor ağırlık merkezi CG(t) (mm)'},
     {value:'thrust',label:'İtki F(t) (N)'},
    ]} onChange={v=>setSeriesView(v as SeriesView)}/>
    <div style={{display:'flex',alignItems:'flex-end'}}>
     <Button variant="ghost" onClick={()=>download(`${motor.name}-zaman-serisi.csv`,motorTimeSeriesCsv(timeSeries),'text/csv')}>Motor Zaman Serisi İndir (CSV)</Button>
    </div>
   </div>
   <div style={{height:190,marginBottom:8}}>
    <Plot title={`${motor.name} zaman serisi`} xLabel="t · s" yLabel={seriesView==='thrust'?'F · N':seriesView==='mdot'?'ṁ · kg/s':seriesView==='cg'?'CG · mm':'Kütle · kg'} series={plotSeries}/>
   </div>
  </details>

  <details className="engineering-details">
   <summary>Katı / Sıvı / Hibrit Motor Ön Tasarım Sihirbazı (Mİ-009 / Mİ-010 · ÖTR s.36–43)</summary>
   <div className="engineering-readiness" role="note" style={{marginBottom:8}}>
    <span><strong>Mİ-010 Mühendislik Uyarısı:</strong> Bu modül ön tasarım düzeyinde çalışır (8.000–100.000 N·s toplam impuls, ince cidarlı tank/gövde mukavemeti FS ≥ 1,5 ve izantropik lüle boğaz hesabı). Tam ölçekli CFD, yanma kararsızlığı ve enjektör atomizasyon optimizasyonu içermez.</span>
   </div>
   {prelimResult.warnings.map(w=><p key={w} role="alert" className="series-error">{w}</p>)}
   <div className="engineering-field-grid">
    <TextField label="Motor adı" value={designInputs.name} onChange={ev=>setDesignInputs(p=>({...p,name:ev.target.value}))}/>
    <Choice label="İtki tipi & yakıt çifti" value={designInputs.presetId} options={(Object.keys(PROPELLANT_PRESETS) as PropellantPresetId[]).map(k=>({value:k,label:PROPELLANT_PRESETS[k].label}))} onChange={v=>updatePreset(v as PropellantPresetId)}/>
    <NumberInput label="Hedef toplam impuls (It)" unit="N·s" min={500} max={250000} value={designInputs.targetImpulseNs} onChange={v=>setDesignInputs(p=>({...p,targetImpulseNs:v??25000}))}/>
    <NumberInput label="Yanma süresi (tb)" unit="s" min={0.5} max={120} value={designInputs.burnTimeS} onChange={v=>setDesignInputs(p=>({...p,burnTimeS:v??8.5}))}/>
    <NumberInput label="Özgül itki (Isp)" unit="s" min={80} max={450} value={designInputs.ispS} onChange={v=>setDesignInputs(p=>({...p,ispS:v??220}))}/>
    <NumberInput label="O/F karışım oranı" unit="—" min={0.5} max={12} value={designInputs.ofRatio} onChange={v=>setDesignInputs(p=>({...p,ofRatio:v??6.5}))}/>
    <NumberInput label="Gövde dış çapı" unit="mm" min={40} max={600} value={designInputs.outerDiameterMm} onChange={v=>setDesignInputs(p=>({...p,outerDiameterMm:v??150}))}/>
    <NumberInput label="Yanma odası basıncı (Pc)" unit="bar" min={5} max={150} value={designInputs.chamberPressureBar} onChange={v=>setDesignInputs(p=>({...p,chamberPressureBar:v??35}))}/>
    <NumberInput label="Tank / gövde basıncı (Ptank)" unit="bar" min={5} max={200} value={designInputs.tankPressureBar} onChange={v=>setDesignInputs(p=>({...p,tankPressureBar:v??55}))}/>
    <NumberInput label="Malzeme izin gerilmesi (σ_izin)" unit="MPa" min={50} max={1500} value={designInputs.allowableStressMpa} onChange={v=>setDesignInputs(p=>({...p,allowableStressMpa:v??276}))}/>
    <NumberInput label="Emniyet katsayısı (FS ≥ 1,5)" unit="—" min={1.0} max={5.0} value={designInputs.safetyFactor} onChange={v=>setDesignInputs(p=>({...p,safetyFactor:v??1.8}))}/>
    <Choice label="İtki profili şekli S(t/tb)" value={designInputs.profileShape} options={[
     {value:'regressive',label:'Azalan (Regressive)'},
     {value:'neutral',label:'Nötr (Neutral)'},
     {value:'progressive',label:'Artan (Progressive)'},
     {value:'trapezoid',label:'Trapez'},
    ]} onChange={v=>setDesignInputs(p=>({...p,profileShape:v as ThrustProfileShape}))}/>
   </div>
   <div className="engineering-summary-grid" style={{marginTop:8,marginBottom:8}}>
    <Metric label="Ortalama / Tepe itki" unit="N" value={`${fmt(prelimResult.averageThrustN,0)} / ${fmt(prelimResult.peakThrustN,0)}`}/>
    <Metric label="Toplam yakıt (m_prop)" unit="kg" value={fmt(prelimResult.totalPropellantMassKg,2)}/>
    <Metric label="Oksitleyici / Yakıt" unit="kg" value={`${fmt(prelimResult.oxidizerMassKg,2)} / ${fmt(prelimResult.fuelMassKg,2)}`}/>
    <Metric label="Toplam debi (ṁ)" unit="kg/s" value={fmt(prelimResult.massFlowRateKgS,3)}/>
    <Metric label="Tank hacmi (V_tank)" unit="L" value={fmt(prelimResult.tankVolumeL,2)}/>
    <Metric label="Asgari cidar (t_cidar)" unit="mm" value={fmt(prelimResult.minWallThicknessMm,2)}/>
    <Metric label="Boğaz / Çıkış çapı" unit="mm" value={`${fmt(prelimResult.throatDiameterMm,1)} / ${fmt(prelimResult.exitDiameterMm,1)}`}/>
    <Metric label="Tahmini boy / Toplam kütle" unit="mm · kg" value={`${prelimResult.estimatedLengthMm} mm · ${fmt(prelimResult.totalMotorMassKg,2)} kg`}/>
   </div>
   {onCreateMotor&&(
    <div style={{marginBottom:8}}>
     <Button onClick={()=>onCreateMotor(prelimResult.motor)}>Ön Tasarım Motorunu Kütüphaneye Ekle ve Seç</Button>
    </div>
   )}
   {catalogMatches.length>0&&(
    <div style={{maxHeight:150,overflowY:'auto'}}>
     <table className="engineering-table">
      <thead>
       <tr>
        <th>En yakın katalog motoru</th>
        <th>İmpuls (N·s)</th>
        <th>Yanış (s)</th>
        <th>Hata skoru (E)</th>
        <th>Benzerlik (%)</th>
       </tr>
      </thead>
      <tbody>
       {catalogMatches.map(c=>(
        <tr key={c.motor.id}>
         <td>{c.motor.name} ({c.motor.manufacturer})</td>
         <td>{fmt(c.impulseNs,0)}</td>
         <td>{fmt(c.burnTimeS,2)}</td>
         <td>{fmt(c.errorScore,3)}</td>
         <td>%{fmt(c.similarityPct,1)}</td>
        </tr>
       ))}
      </tbody>
     </table>
    </div>
   )}
  </details>

  <details className="engineering-details"><summary>Hesap modeli</summary><p>Parçalı doğrusal itki, impuls oranından anlık yakıt tüketimi, kütle debisi ṁ(t), özgül itki Isp ve zamanla değişen motor ağırlık merkezi CG(t). Ön tasarım modülü ÖTR s.36–43 denklemlerini kullanır.</p>{result&&<span>{result.snapshot.updated.length} node çalıştı · native C++</span>}</details>
 </div>;
}
