import {useEffect,useState} from 'react';
import {ChevronDown,ChevronUp} from 'lucide-react';
import type {Project,Part} from '../domain/project';
import {resolveParts,boundsOf} from '../domain/geometry';
import {isStructural} from '../domain/componentCatalog';
export default function CanvasSummary({project,focus,selection,onSelect,onFocus}:{project:Project;focus:string;selection:string[];onSelect:(id:string)=>void;onFocus:()=>void}){
 const [expanded,setExpanded]=useState(false);const root=project.parts.find(p=>p.kind==='rocket')!;
 const selected=selection.map(id=>project.parts.find(p=>p.id===id)).filter((p):p is Part=>!!p);
 const whole=focus===root.id&&(selected.length===0||selected.length===1&&selected[0].id===root.id);
 const parts=selected.length?selected:[project.parts.find(p=>p.id===focus)!];
 useEffect(()=>setExpanded(false),[focus,selection.join(':')]);
 if(whole){const bounds=boundsOf(resolveParts(project));const provided=root.inputs?.massProvided===true||root.inputs?.massProvided!==false&&root.mass>0;
  return <aside className="canvas-summary" aria-label="Roket özeti" onPointerDown={e=>e.stopPropagation()}><dl><dt>Uzunluk</dt><dd>{Math.round(bounds.max[0]-bounds.min[0])} mm</dd><dt title={provided?'Kullanıcının girdiği roket kütlesi':'Toplam kütle hesabı henüz bağlı değil'}>{provided?'Girilen kütle':'Kütle'}</dt><dd>{provided?`${root.mass.toLocaleString('tr-TR')} g`:'—'}</dd></dl><span>{project.parts.filter(p=>p.kind==='stage'||p.kind==='parallelStage').length} kademe · {project.parts.filter(p=>!isStructural(p.kind)).length} parça</span></aside>;
 }
 return <aside className="canvas-summary selection-summary" aria-label="Görünüm seçimi" onPointerDown={e=>e.stopPropagation()}>
 <button className="summary-selection-title" title={parts.map(p=>p.name).join('\n')} aria-expanded={expanded} onClick={()=>parts.length>1?setExpanded(v=>!v):onFocus()}><span>{parts[0]?.name}{parts.length>1?` +${parts.length-1}`:''}</span>{parts.length>1&&(expanded?<ChevronUp size={13}/>:<ChevronDown size={13}/>)}</button>
 {expanded&&<div className="summary-selection-list">{parts.map(p=><button key={p.id} title={p.name} onClick={()=>onSelect(p.id)}>{p.name}</button>)}</div>}
 </aside>;
}
