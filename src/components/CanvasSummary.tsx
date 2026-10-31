import {useEffect,useState} from 'react';
import {ChevronDown,ChevronUp} from '@stardrop/ui/icons';
import type {Project,Part} from '../domain/project';
import {resolveParts,boundsOf} from '../domain/geometry';
import {isStructural} from '../domain/componentCatalog';
import type {DesignAnalysisState} from './useDesignAnalysis';
export default function CanvasSummary({project,focus,selection,onSelect,onFocus,analysis}:{project:Project;focus:string;selection:string[];onSelect:(id:string)=>void;onFocus:()=>void;analysis?:DesignAnalysisState}){
 const [expanded,setExpanded]=useState(false);const root=project.parts.find(p=>p.kind==='rocket')!;
 const selected=selection.map(id=>project.parts.find(p=>p.id===id)).filter((p):p is Part=>!!p);
 const whole=focus===root.id&&(selected.length===0||selected.length===1&&selected[0].id===root.id);
 const parts=selected.length?selected:[project.parts.find(p=>p.id===focus)!];
 useEffect(()=>setExpanded(false),[focus,selection.join(':')]);
 if(whole){const bounds=boundsOf(resolveParts(project)),result=analysis?.result,title=analysis?.issue??(analysis?.pending?'Hesaplanıyor':'');
  return <aside className="canvas-summary" aria-label="Roket özeti" title={title} onPointerDown={e=>e.stopPropagation()}><dl><dt>Uzunluk</dt><dd>{Math.round(bounds.max[0]-bounds.min[0])} mm</dd><dt>Kütle</dt><dd>{result?`${(result.mass*1000).toLocaleString('tr-TR',{maximumFractionDigits:1})} g`:'—'}</dd><dt>CG</dt><dd>{result?`${(result.cg*1000).toLocaleString('tr-TR',{maximumFractionDigits:1})} mm`:'—'}</dd><dt>CP</dt><dd>{result?`${(result.cp*1000).toLocaleString('tr-TR',{maximumFractionDigits:1})} mm`:'—'}</dd><dt>Marj</dt><dd>{result?`${result.margin.toLocaleString('tr-TR',{maximumFractionDigits:2})} cal`:'—'}</dd></dl><span>{project.parts.filter(p=>p.kind==='stage'||p.kind==='parallelStage').length} kademe · {project.parts.filter(p=>!isStructural(p.kind)).length} parça</span></aside>;
 }
 return <aside className="canvas-summary selection-summary" aria-label="Görünüm seçimi" onPointerDown={e=>e.stopPropagation()}>
 <button className="summary-selection-title" title={parts.map(p=>p.name).join('\n')} aria-expanded={expanded} onClick={()=>parts.length>1?setExpanded(v=>!v):selected.length?onFocus():onSelect(parts[0].id)}><span>{parts[0]?.name}{parts.length>1?` +${parts.length-1}`:''}</span>{parts.length>1&&(expanded?<ChevronUp size={13}/>:<ChevronDown size={13}/>)}</button>
 {expanded&&<div className="summary-selection-list">{parts.map(p=><button key={p.id} title={p.name} onClick={()=>onSelect(p.id)}>{p.name}</button>)}</div>}
 </aside>;
}
