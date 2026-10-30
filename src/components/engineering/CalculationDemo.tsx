import {useEffect,useRef,useState} from 'react';
import {IconButton,SelectMenu,NodeCard} from '@stardrop/ui';
import {Maximize,Minus,Plus,RefreshCw} from '@stardrop/ui/icons';
import type {Part,Project} from '../../domain/project';
import type {EngineResponse,EngineProgress} from '../../engine/protocol';
import {evaluateStructure} from '../../engine/client';
import {hasStructuralModel,structuralInputs} from '../../engine/structuralInputs';
import {NumberInput} from '../NumberInput';
import {useCanvasNavigation} from '../useCanvasNavigation';
import ResizableSplit from '../ResizableSplit';
import {Empty} from './shared';
import './calculation-demo.css';

export default function CalculationDemo({project,onChange}:{project:Project;onChange:(id:string,patch:Partial<Part>)=>void}){
 const bodies=project.parts.filter(hasStructuralModel);const [partId,setPartId]=useState(''),[response,setResponse]=useState<EngineResponse>(),[error,setError]=useState(''),[running,setRunning]=useState(false),[zoom,setZoom]=useState(1),[selected,setSelected]=useState('mass');
 const part=bodies.find(p=>p.id===partId)??bodies[0];const session=useRef(new Map<string,string>());if(part&&!session.current.has(part.id))session.current.set(part.id,crypto.randomUUID());const [retry,setRetry]=useState(0),[progress,setProgress]=useState<EngineProgress>();
 const graph=response?.manifest??{sources:[],nodes:[]},snapshot=response?.snapshot;
 const request=part?structuralInputs(part):undefined,inputs=request?.inputs??{},issue=request?.issue;const inputKey=JSON.stringify([request?.model,inputs]);
 useEffect(()=>{const controller=new AbortController();if(!part)return;if(issue){setResponse(undefined);setError(issue);setRunning(false);return}setRunning(true);setError('');setProgress(undefined);void evaluateStructure({protocolVersion:1,model:request!.model,requestId:crypto.randomUUID(),sessionId:session.current.get(part.id)!,inputs},controller.signal,event=>{if(!controller.signal.aborted)setProgress(event)}).then(result=>{if(!controller.signal.aborted){setResponse(result);setRunning(false)}}).catch(e=>{if(!controller.signal.aborted){setResponse(undefined);setError(e instanceof Error?e.message:'Hesap tamamlanamadı.');setRunning(false)}});return()=>controller.abort()},[inputKey,issue,part?.id,retry]);
 useEffect(()=>{const refresh=()=>setRetry(v=>v+1),failed=(event:{message:string})=>{setResponse(undefined);setError(event.message);setRunning(false)};import.meta.hot?.on('starlab:engine-restarted',refresh);import.meta.hot?.on('starlab:engine-error',failed);return()=>{import.meta.hot?.off('starlab:engine-restarted',refresh);import.meta.hot?.off('starlab:engine-error',failed)}},[]);
 const items=[...graph.sources,...graph.nodes],levels=new Map(graph.sources.map(s=>[s.id,0]));
 for(const n of graph.nodes)levels.set(n.id,1+Math.max(0,...Object.values(n.inputs).map(d=>levels.get(d.id)??0)));
 const columns=Array.from({length:Math.max(0,...levels.values())+1},(_,i)=>items.filter(n=>levels.get(n.id)===i));
 const height=Math.max(300,Math.max(...columns.map(c=>c.length))*108+40),width=columns.length*280+40;
 const positions=new Map(columns.flatMap((col,x)=>col.map((item,y)=>[item.id,{x:20+x*280,y:(height-col.length*108)/2+y*108}] as const)));
 const svg=useRef<SVGSVGElement>(null),navigation=useCanvasNavigation(svg,{x:0,y:0,width,height},zoom,setZoom,part?.id??'',false,Boolean(response));
 const [pan,setPan]=useState({x:0,y:0}),drag=useRef<{x:number;y:number;dx:number;dy:number;pan:{x:number;y:number}}>();
 // Pan by pointer dragging; touchpad wheel/pinch uses the shared canvas hook.
 const entry=items.find(n=>n.id===selected),node=graph.nodes.find(n=>n.id===selected),active=snapshot?.results[selected];
 const format=(value:unknown)=>typeof value==='number'?value.toLocaleString('tr-TR',{maximumSignificantDigits:7}):JSON.stringify(value);
 const frame=navigation.frame;
 if(!part)return <Empty>Tasarıma gövde, iç tüp veya kütle bileşeni ekle</Empty>;
 
 const fields=<div className="engineering-body input-settings calculation-inputs"><SelectMenu label="Hesaplanacak bileşen" value={part.id} options={bodies.map(p=>({value:p.id,label:p.name}))} onChange={id=>{setPartId(id);setResponse(undefined);setPan({x:0,y:0})}}/>
  <div className="engineering-field-grid">
   <NumberInput label="Uzunluk" unit="mm" value={part.length} min={Number.MIN_VALUE} onChange={v=>onChange(part.id,{length:v!})}/>
   <NumberInput label="Dış çap" unit="mm" value={part.diameter} min={Math.max(Number.MIN_VALUE,part.geometry?.innerDiameter??0,2*(part.inputs?.wallThickness??0))} onChange={v=>onChange(part.id,{diameter:v!,...(part.kind==='innerTube'&&part.geometry?.innerDiameter!==undefined&&part.inputs?.wallThickness!==undefined?{inputs:{wallThickness:(v!-part.geometry.innerDiameter)/2}}:{})})}/>
   {part.kind==='innerTube'&&<NumberInput label="İç çap" unit="mm" optional value={part.geometry?.innerDiameter} min={Number.MIN_VALUE} max={part.diameter} onChange={v=>onChange(part.id,{geometry:{...part.geometry,innerDiameter:v},...(v!==undefined&&part.inputs?.wallThickness!==undefined?{inputs:{wallThickness:(part.diameter-v)/2}}:{})})}/>}
   {part.kind==='mass'?<NumberInput label="Kütle" unit="g" optional value={inputs.suppliedMass===undefined?undefined:part.mass} min={Number.MIN_VALUE} onChange={v=>onChange(part.id,{mass:v??0,inputs:{massProvided:v!==undefined}})}/>:<>
    <NumberInput label="Et kalınlığı" unit="mm" optional value={part.inputs?.wallThickness} max={part.diameter/2} onChange={v=>onChange(part.id,{inputs:{wallThickness:v},...(part.kind==='innerTube'&&v!==undefined?{geometry:{...part.geometry,innerDiameter:part.diameter-2*v}}:{})})}/>
    <NumberInput label="Yoğunluk" unit="kg/m³" optional min={Number.MIN_VALUE} value={part.inputs?.density} onChange={v=>onChange(part.id,{inputs:{density:v}})}/>
   </>}
  </div>
  <div className="calculation-result" aria-label="Seçili hesap"><strong>{entry?.label}</strong>{running?<span>Hesaplanıyor…</span>:active?.state.status==='ready'?<span>{format(active.state.value)} <small>{entry?.output.unit}</small></span>:<span>{active?.state.status==='missing'?'Girdi gerekli':active?.state.status==='blocked'?'Girdi bekleniyor':active?.state.status==='error'?active.state.error:'—'}</span>}
   {node&&<><span>{active?.runs??0} çalıştırma · v{node.version}</span><div>{Object.entries(node.inputs).map(([name,ref])=><button key={name} onClick={()=>setSelected(ref.id)}>{items.find(n=>n.id===ref.id)?.label} <small>{ref.output.unit}</small></button>)}</div><span title={node.provenance}>{node.provenance}</span><span>{node.implementation}</span></>}
  </div><details className="engineering-details"><summary>Model kapsamı</summary><p className="calculation-scope">{part.kind==='mass'?'RoketSiM MassComponent: girilen kütleyle homojen dolu silindir.':'RoketSiM: tek, homojen tüp.'} Alt bileşenler ve kütle/CG düzeltmeleri bu hesaba dahil değil; bu değerler tüm roketin toplamı değil. CG bileşen başlangıcından; ataletler yerel CG etrafında. Sonuçlar SI biriminde.</p></details>
 </div>;
 const canvas=!response?<Empty>{error||'Native hesap motoruna bağlanıyor…'}{error&&!issue&&<IconButton variant="ghost" label="Hesap motoruna yeniden bağlan" onClick={()=>setRetry(v=>v+1)}><RefreshCw size={18}/></IconButton>}</Empty>:<div className="calculation-canvas"><div className="calculation-toolbar"><span aria-live="polite">{running?(progress?`${items.find(n=>n.id===progress.nodeId)?.label??progress.nodeId}…`:'Hesaplanıyor'):`Son işlem · ${snapshot?.updated.length??0} node`}</span><div><IconButton variant="ghost" size="sm" label="Hesap ağını uzaklaştır" onClick={()=>setZoom(z=>Math.max(.25,z/1.2))}><Minus size={15}/></IconButton><IconButton variant="ghost" size="sm" label="Hesap ağını yakınlaştır" onClick={()=>setZoom(z=>Math.min(8,z*1.2))}><Plus size={15}/></IconButton><IconButton variant="ghost" size="sm" label="Hesap ağını sığdır" onClick={()=>{navigation.reset();setPan({x:0,y:0})}}><Maximize size={15}/></IconButton></div></div>
  <svg ref={svg} className="calculation-map" viewBox={`${frame.x+pan.x} ${frame.y+pan.y} ${frame.width} ${frame.height}`} aria-label="Modüler hesaplama ağı" onPointerDown={e=>{if((e.target as Element).closest('[data-node]'))return;const matrix=svg.current?.getScreenCTM()?.inverse();if(!matrix)return;drag.current={x:e.clientX,y:e.clientY,dx:matrix.a,dy:matrix.d,pan};e.currentTarget.setPointerCapture(e.pointerId)}} onPointerMove={e=>{if(!drag.current)return;const start=drag.current;setPan({x:start.pan.x-(e.clientX-start.x)*start.dx,y:start.pan.y-(e.clientY-start.y)*start.dy})}} onPointerUp={()=>{drag.current=undefined}} onPointerCancel={()=>{drag.current=undefined}}>
   <defs><marker id="calc-arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="5" markerHeight="5" orient="auto"><path d="M0 0L10 5L0 10" fill="none" stroke="white"/></marker></defs>
   {graph.nodes.flatMap(n=>Object.values(n.inputs).map(ref=>{const a=positions.get(ref.id)!,b=positions.get(n.id)!;return <path key={`${ref.id}-${n.id}`} className={`calculation-edge ${snapshot?.changed.includes(ref.id)?'changed':''}`} d={`M${a.x+220},${a.y+40}C${a.x+250},${a.y+40} ${b.x-30},${b.y+40} ${b.x},${b.y+40}`} markerEnd="url(#calc-arrow)"/>}))}
   {items.map(item=>{const pos=positions.get(item.id)!,result=snapshot?.results[item.id];return <foreignObject key={item.id} x={pos.x} y={pos.y} width="220" height="84"><NodeCard data-node={item.id} className={`calculation-node ${result?.action==='computed'?'computed':''}`} selected={selected===item.id} onClick={()=>setSelected(item.id)} title={item.label} value={<span title={result?.state.status==='ready'?`${format(result.state.value)} ${item.output.unit}`:result?.state.error}>{running?'…':result?.state.status==='ready'?`${format(result.state.value)} ${item.output.unit}`:'—'}</span>} status={result?.state.status==='error'?'Hata':result?.state.status==='missing'?'Eksik':result?.state.status==='blocked'?'Bekliyor':result?.action==='computed'?'Çalıştı':result?.action==='cached'?'Önbellek':'Girdi'}/></foreignObject>})}
  </svg>{error&&<p className="series-error" role="alert">{error}</p>}
 </div>;
 return <ResizableSplit className="engineering-split" axis="x" storageKey="calculations.graph" label="Hesap girdileri ve ağ genişliği" initialRatio={.3} minFirst={300} minSecond={350}>{fields}{canvas}</ResizableSplit>;
}
