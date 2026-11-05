import React,{Suspense,useEffect,useMemo,useState} from 'react';
import {Button,SelectMenu} from '@stardrop/ui';
import {Download,Play} from '@stardrop/ui/icons';
import type {Environment,FlightSample} from '../../domain/flight';
import {bearingFromEnu,landingZoneCsv,singleFlightLandingZone,type LandingZoneResult} from '../../domain/landingZone';
import {Metric,download,fmt} from './shared';

const LandingZone3D=React.lazy(()=>import('./LandingZone3D'));

const WIDTH=1000,HEIGHT=420,PAD=48;

function LandingZone2D({result,selectedRun,showTrajectories,focusZone,onSelectRun}:{result:LandingZoneResult;selectedRun:number;showTrajectories:boolean;focusZone:boolean;onSelectRun:(index:number)=>void}){
 const e=result.ellipse;
 const bounds=useMemo(()=>{
  const pts:{east:number;north:number}[]=focusZone?[]:[{east:0,north:0}];
  for(const p of result.points){
   pts.push({east:p.east,north:p.north});
   if(!focusZone&&showTrajectories)for(const t of p.trajectory)pts.push({east:t.east,north:t.north});
  }
  const margin=Math.max(15,e.semiMajor3Sigma*1.25);
  pts.push({east:e.centerEast-margin,north:e.centerNorth-margin},{east:e.centerEast+margin,north:e.centerNorth+margin});
  const easts=pts.map(p=>p.east),norths=pts.map(p=>p.north);
  const minE=Math.min(...easts),maxE=Math.max(...easts),minN=Math.min(...norths),maxN=Math.max(...norths);
  const spanE=Math.max(20,maxE-minE),spanN=Math.max(20,maxN-minN);
  const scale=Math.min((WIDTH-PAD*2)/spanE,(HEIGHT-PAD*2)/spanN);
  const midE=(minE+maxE)/2,midN=(minN+maxN)/2;
  const x=(east:number)=>WIDTH/2+(east-midE)*scale;
  const y=(north:number)=>HEIGHT/2-(north-midN)*scale;
  const rawBar=120/scale,mag=Math.pow(10,Math.floor(Math.log10(rawBar)));
  const barM=rawBar/mag>=5?5*mag:rawBar/mag>=2?2*mag:mag;
  return {x,y,scale,barM,barPx:barM*scale};
 },[result,focusZone,showTrajectories,e]);

 const cx=bounds.x(e.centerEast),cy=bounds.y(e.centerNorth);
 const padX=bounds.x(0),padY=bounds.y(0);
 const rx1=e.semiMajor1Sigma*bounds.scale,ry1=e.semiMinor1Sigma*bounds.scale;
 const rx2=e.semiMajor2Sigma*bounds.scale,ry2=e.semiMinor2Sigma*bounds.scale;
 const rx3=e.semiMajor3Sigma*bounds.scale,ry3=e.semiMinor3Sigma*bounds.scale;
 const svgAngle=-e.angleDeg;

 return <svg viewBox={`0 0 ${WIDTH} ${HEIGHT}`} role="img" aria-label="Doğu ve kuzey eksenlerinde 2D iniş bölgesi ve dağılım elipsleri">
  {!focusZone&&[0.33,0.66,1].map(f=>{
   const r=Math.max(10,e.maxRangeM*f*bounds.scale);
   return <circle key={f} cx={padX} cy={padY} r={r} fill="none" stroke="#ffffff14" strokeDasharray="3 6"/>;
  })}
  <line x1={padX} y1={16} x2={padX} y2={HEIGHT-16} stroke="#ffffff14"/>
  <line x1={16} y1={padY} x2={WIDTH-16} y2={padY} stroke="#ffffff14"/>
  {showTrajectories&&result.points.map(p=>{
   if(p.trajectory.length<2)return null;
   const active=p.index===selectedRun;
   const d=p.trajectory.map((t,i)=>`${i?'L':'M'}${bounds.x(t.east).toFixed(1)} ${bounds.y(t.north).toFixed(1)}`).join(' ');
   return <path key={`path-${p.index}`} d={d} fill="none" stroke="#fff" strokeOpacity={active?0.95:p.nominal?0.6:0.18} strokeWidth={active?2:1.2}/>;
  })}
  {!focusZone&&(result.bodies??[]).filter(b=>b.ellipse!==e&&b.ellipse.semiMajor1Sigma>0.5).map(b=>{
   const bx=bounds.x(b.ellipse.centerEast),by=bounds.y(b.ellipse.centerNorth);
   const brx3=b.ellipse.semiMajor3Sigma*bounds.scale,bry3=b.ellipse.semiMinor3Sigma*bounds.scale;
   return <g key={`other-${b.id}`} transform={`translate(${bx} ${by}) rotate(${-b.ellipse.angleDeg})`}>
    <ellipse cx={0} cy={0} rx={brx3} ry={bry3} fill="none" stroke="#ffffff66" strokeWidth={1.2} strokeDasharray="2 5"/>
    <circle cx={0} cy={0} r={3} fill="#fff"/>
    <text x={8} y={-6}>{b.name}</text>
   </g>;
  })}
  {rx1>0.5&&<g transform={`translate(${cx} ${cy}) rotate(${svgAngle})`}>
   <ellipse cx={0} cy={0} rx={rx3} ry={ry3} fill="#ffffff08" stroke="#ffffff55" strokeWidth={1.2} strokeDasharray="6 4"/>
   <ellipse cx={0} cy={0} rx={rx2} ry={ry2} fill="#ffffff0c" stroke="#ffffff88" strokeWidth={1.3} strokeDasharray="3 3"/>
   <ellipse cx={0} cy={0} rx={rx1} ry={ry1} fill="#ffffff16" stroke="#fff" strokeWidth={1.8}/>
   <line x1={-rx3} y1={0} x2={rx3} y2={0} stroke="#ffffff40" strokeWidth={1}/>
   <line x1={0} y1={-ry3} x2={0} y2={ry3} stroke="#ffffff40" strokeWidth={1}/>
  </g>}
  {!focusZone&&<g transform={`translate(${padX} ${padY})`}>
   <circle r={5} fill="#000" stroke="#fff" strokeWidth={2}/>
   <text x={10} y={-8}>Fırlatma (0,0)</text>
  </g>}
  {result.points.map(p=>{
   const active=p.index===selectedRun;
   const px=bounds.x(p.east),py=bounds.y(p.north);
   return <g key={`imp-${p.index}`} style={{cursor:'pointer'}} onClick={()=>onSelectRun(p.index)}>
    <circle cx={px} cy={py} r={active?6.5:p.nominal?5:3.8} fill={p.nominal?'#000':'#fff'} fillOpacity={active||p.nominal?1:0.75} stroke="#fff" strokeWidth={active?2.2:1.4}>
     <title>{p.nominal?'Nominal iniş':`Koşu #${p.index}`} · D {fmt(p.east,1)} m · K {fmt(p.north,1)} m</title>
    </circle>
   </g>;
  })}
  <text x={24} y={28}>Kuzey ↑</text>
  <text x={WIDTH-24} y={28} textAnchor="end">{result.points.length>1?`${result.points.length-1} dağılım koşusu · 1σ / 2σ / 3σ`:'Tekil uçuş iniş noktası'}</text>
  <g transform={`translate(24 ${HEIGHT-22})`}>
   <line x1={0} y1={0} x2={bounds.barPx} y2={0} stroke="#fff" strokeWidth={2}/>
   <line x1={0} y1={-4} x2={0} y2={4} stroke="#fff" strokeWidth={2}/>
   <line x1={bounds.barPx} y1={-4} x2={bounds.barPx} y2={4} stroke="#fff" strokeWidth={2}/>
   <text x={bounds.barPx+10} y={4}>{fmt(bounds.barM,0)} m</text>
  </g>
  <text x={WIDTH-24} y={HEIGHT-18} textAnchor="end">Doğu →</text>
 </svg>;
}

export default function LandingZoneView({samples,environment,landingZone,canRunDispersion,runningDispersion,onRunDispersion}:{samples:FlightSample[];environment:Environment;landingZone?:LandingZoneResult;canRunDispersion:boolean;runningDispersion:boolean;onRunDispersion:()=>void}){
 const [dimension,setDimension]=useState<'3d'|'2d'>('3d');
 const [focusZone,setFocusZone]=useState(false);
 const [showTrajectories,setShowTrajectories]=useState(true);
 const [selectedRun,setSelectedRun]=useState(0);
 const [selectedBodyId,setSelectedBodyId]=useState<'sustainer'|'payload'|'booster'>('sustainer');

 const baseZone=useMemo(()=>landingZone??singleFlightLandingZone(samples,environment),[landingZone,samples,environment]);
 const activeBody=baseZone?.bodies?.find(b=>b.id===selectedBodyId)??baseZone?.bodies?.[0];
 const activeZone=useMemo(()=>baseZone&&activeBody?{...baseZone,ellipse:activeBody.ellipse,points:activeBody.points}:baseZone,[baseZone,activeBody]);
 useEffect(()=>{
  if(!activeZone||activeZone.points.length<=1)return;
  const onKeyDown=(event:KeyboardEvent)=>{
   if(event.key!=='Tab'||event.altKey||event.ctrlKey||event.metaKey)return;
   const target=event.target as HTMLElement|null;
   if(target&&(target.tagName==='INPUT'||target.tagName==='TEXTAREA'||target.isContentEditable))return;
   event.preventDefault();
   setSelectedRun(current=>{
    const pts=activeZone.points;
    const idx=pts.findIndex(p=>p.index===current);
    const step=event.shiftKey?-1:1;
    const nextIdx=idx<0?0:(idx+step+pts.length)%pts.length;
    return pts[nextIdx].index;
   });
  };
  window.addEventListener('keydown',onKeyDown);
  return()=>window.removeEventListener('keydown',onKeyDown);
 },[activeZone]);
 if(!activeZone)return <div className="engineering-empty">İniş bölgesi için önce simülasyonu çalıştır veya dağılımı hesapla</div>;

 const selectedPoint=activeZone.points.find(p=>p.index===selectedRun)??activeZone.points[0];
 const e=activeZone.ellipse;
 const hasDispersion=activeZone.points.length>1;

 return <div className="landing-zone-view">
  <div className="landing-zone-toolbar">
   <div className="view-switch" role="group" aria-label="İniş bölgesi görünüm boyutu">
    <button type="button" aria-pressed={dimension==='3d'} onClick={()=>setDimension('3d')}>3D sahne</button>
    <button type="button" aria-pressed={dimension==='2d'} onClick={()=>setDimension('2d')}>2D harita</button>
   </div>
   <div className="view-switch" role="group" aria-label="Kadraj ve yörünge">
    <button type="button" aria-pressed={!focusZone} onClick={()=>setFocusZone(false)}>Genel saha</button>
    <button type="button" aria-pressed={focusZone} onClick={()=>setFocusZone(true)}>İniş odağı</button>
    <button type="button" aria-pressed={showTrajectories} onClick={()=>setShowTrajectories(v=>!v)}>Yörüngeler</button>
   </div>
   <div className="landing-zone-actions">
    {(baseZone?.bodies?.length??0)>1&&<SelectMenu label="Kurtarılan bileşen" value={activeBody?.id??'sustainer'} options={baseZone!.bodies!.map(b=>({value:b.id,label:b.name}))} onChange={v=>setSelectedBodyId(v as 'sustainer'|'payload'|'booster')}/>}
    {activeZone.points.length>1&&<SelectMenu label="Seçili koşu" value={String(selectedPoint.index)} options={activeZone.points.map(p=>({value:String(p.index),label:p.nominal?'Nominal uçuş':`Koşu #${p.index} (${fmt(Math.hypot(p.east,p.north),0)} m)`}))} onChange={v=>setSelectedRun(Number(v))}/>}
    {canRunDispersion&&<Button size="sm" variant="secondary" disabled={runningDispersion} onClick={onRunDispersion}><Play size={14}/>{runningDispersion?'Dağılım hesaplanıyor…':hasDispersion?'Dağılımı yenile':'Dağılımı hesapla'}</Button>}
    <Button size="sm" variant="ghost" onClick={()=>download('starlab-inis-bolgesi.csv',landingZoneCsv(activeZone),'text/csv')}><Download size={14}/>CSV</Button>
   </div>
  </div>
  <div className="flight-path landing-zone-canvas">
   {dimension==='3d'
    ?<Suspense fallback={<div className="engineering-empty">3D sahne yükleniyor…</div>}>
     <LandingZone3D result={activeZone} selectedRun={selectedPoint.index} showTrajectories={showTrajectories} focusZone={focusZone} onSelectRun={setSelectedRun} onFallback={()=>setDimension('2d')}/>
    </Suspense>
    :<LandingZone2D result={activeZone} selectedRun={selectedPoint.index} showTrajectories={showTrajectories} focusZone={focusZone} onSelectRun={setSelectedRun}/>}
  </div>
  <div className="engineering-metrics landing-zone-metrics">
   <Metric label="Merkez menzil · yön" value={`${fmt(e.meanRangeM,1)} m · ${fmt(e.meanBearingDeg,0)}°`} unit=""/>
   <Metric label="Merkez konum (D / K)" value={`${fmt(e.centerEast,1)} / ${fmt(e.centerNorth,1)}`} unit="m"/>
   <Metric label="CEP50 yarıçapı" value={hasDispersion?fmt(e.cep50Radius,1):'—'} unit="m"/>
   <Metric label="3σ elips (a × b)" value={hasDispersion?`${fmt(e.semiMajor3Sigma,1)} × ${fmt(e.semiMinor3Sigma,1)}`:'—'} unit="m"/>
   <Metric label={selectedPoint.nominal?'Nominal temas hızı':`Koşu #${selectedPoint.index} temas hızı`} value={fmt(selectedPoint.impactSpeed,1)} unit="m/s"/>
  </div>
 </div>;
}
