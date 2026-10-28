import { useMemo,useEffect,useRef,useState,type KeyboardEvent } from 'react';
import { resolveParts,scopeParts,boundsOf,noseProfile,finProfile } from '../domain/geometry';
import type { ViewProps } from './ViewProps';
import type {Part} from '../domain/project';
import {geometryPatch,type GeometryEdit,type GeometryPatch} from '../domain/geometryEdit';
import {useGeometryDrag} from './useGeometryDrag';
import {useCanvasNavigation} from './useCanvasNavigation';
interface Props extends ViewProps {onEdit?:(id:string,patch:GeometryPatch)=>void;onPreview?:(part:Part|null)=>void;onZoom?:(zoom:number)=>void;resetKey?:number}
export default function RocketView({project,selected,focus,onSelect,zoom,onEdit,onPreview,onZoom,resetKey=0}:Props){
 const svg=useRef<SVGSVGElement>(null);const [size,setSize]=useState({width:1000,height:400});
 const drag=useGeometryDrag(svg,project,focus,onEdit,onPreview);
 const working=useMemo(()=>drag.draft?{...project,parts:project.parts.map(p=>p.id===drag.draft!.id?{...p,...drag.draft!.patch}:p)}:project,[project,drag.draft]);
 const resolved=useMemo(()=>resolveParts(working),[working]);const shown=scopeParts(resolved,focus).filter(n=>n.physical);
 const original=useMemo(()=>scopeParts(resolveParts(project),focus),[project,focus]);const bounds=boundsOf(original);const min=bounds.min[0],max=bounds.max[0];const radius=Math.max(bounds.max[1],100);const height=Math.max(500,radius*4);const width=Math.max(300,max-min)+300;
 const nav=useCanvasNavigation(svg,{x:min-150,y:-height/2,width,height},zoom,onZoom,`${focus}:${resetKey}`,!!drag.draft);
 const unit=Math.max(nav.frame.width/Math.max(size.width,1),nav.frame.height/Math.max(size.height,1));
 useEffect(()=>{const node=svg.current;if(!node)return;const observer=new ResizeObserver(([entry])=>setSize({width:entry.contentRect.width,height:entry.contentRect.height}));observer.observe(node);return()=>observer.disconnect()},[]);
 const current=shown.find(n=>n.part.id===selected);
 const keyEdit=(event:KeyboardEvent<SVGGElement>,part:Part,mode?:GeometryEdit)=>{
  if(event.ctrlKey||event.metaKey||!event.key.startsWith('Arrow'))return;
  const step=event.shiftKey?10:1;let edit=mode;
  if(!edit&&event.altKey)edit=['ArrowLeft','ArrowRight'].includes(event.key)?'length':'diameter';
  if(!edit&&['ArrowUp','ArrowDown'].includes(event.key)){
   event.preventDefault();event.stopPropagation();const index=shown.findIndex(n=>n.part.id===part.id);const next=shown[(index+(event.key==='ArrowDown'?1:-1)+shown.length)%shown.length];
   const element=Array.from(svg.current?.querySelectorAll<SVGGElement>('[data-part-id]')??[]).find(n=>n.dataset.partId===next.part.id);element?.focus();return;
  }
  if(!onEdit)return;event.preventDefault();event.stopPropagation();const positive=['ArrowRight','ArrowUp'].includes(event.key);
  onEdit(part.id,geometryPatch(part,edit??'move',(positive?1:-1)*step));
 };
 const handle=(mode:'length'|'diameter',x:number,y:number)=>current&&<g className={`geometry-handle handle-${mode}`} role="slider" tabIndex={0} aria-label={mode==='length'?'Uzunluğu değiştir':'Çapı değiştir'} aria-valuenow={current.part[mode]} aria-valuemin={1} aria-valuetext={`${current.part[mode]} mm`} aria-orientation={mode==='length'?'horizontal':'vertical'} onPointerDown={event=>drag.start(event,current.part,mode)} onKeyDown={event=>keyEdit(event,current.part,mode)}>
 <title>{mode==='length'?'Uzunluk':'Çap'} · sürükle · Shift: 10 mm · Alt: 0,1 mm</title><rect className="handle-hit" x={x-unit*14} y={y-unit*14} width={unit*28} height={unit*28}/><rect className="handle-mark" x={x-unit*5} y={y-unit*5} width={unit*10} height={unit*10}/></g>;
 return <svg ref={svg} className={`rocket-svg ${onEdit?'editable':''} ${drag.draft?'geometry-dragging':''}`} viewBox={`${nav.frame.x} ${nav.frame.y} ${nav.frame.width} ${nav.frame.height}`} aria-label="Roket teknik kesiti" onPointerMove={drag.move} onPointerUp={()=>drag.finish(true)} onPointerCancel={()=>drag.finish(false)} onLostPointerCapture={()=>drag.finish(false)} onDoubleClick={event=>{if(!(event.target as Element).closest('.rocket-part,.geometry-handle'))nav.reset()}}>
 <defs><pattern id="hatch" width="18" height="18" patternUnits="userSpaceOnUse"><path d="M0 18L18 0" stroke="#fff" strokeWidth="1"/></pattern></defs>
 <line x1={min-150} x2={max+150} y1="0" y2="0" stroke="#fff" strokeWidth="1" strokeDasharray="12 12"/>
 {shown.map(({part:n,position})=>{const x=position[0],d=n.diameter/2;const parent=working.parts.find(p=>p.id===n.parentId);const outline=n.kind==='nose'?noseProfile(n.length,d):finProfile(n.length,d,(parent?.diameter||140)/2);return <g key={n.id} data-part-id={n.id} className={`rocket-part ${selected===n.id?'selected':''} ${['motor','parachute','cord'].includes(n.kind)?'internal':''}`} tabIndex={0} role="button" aria-label={n.name} aria-pressed={selected===n.id} onFocus={()=>onSelect(n.id)} onPointerDown={event=>{if(onEdit){onSelect(n.id);drag.start(event,n,'move')}}} onClick={()=>onSelect(n.id)} onKeyDown={e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();onSelect(n.id)}else keyEdit(e,n)}}><title>{n.name}{onEdit?' · sürükle: konum':''}</title>{n.kind==='nose'?<polygon points={[...outline.map(([a,b])=>`${x+a},${-b}`),...outline.slice().reverse().map(([a,b])=>`${x+a},${b}`)].join(' ')}/>:n.kind==='fins'?<>{[-1,1].map(sign=><polygon key={sign} points={outline.map(([a,b])=>`${x+a},${b*sign}`).join(' ')}/>)}</>:<rect x={x} y={-d} width={Math.max(n.length,1)} height={Math.max(n.diameter,1)} rx={n.kind==='parachute'?15:0}/>}</g>})}
 <path d={`M${min} ${radius+40}v20 H${max}v-20`} fill="none" stroke="#fff" strokeWidth="2"/><text x={(min+max)/2} y={radius+100} textAnchor="middle" fill="#fff" fontSize="24">{Math.round(max-min)} mm</text>
 {onEdit&&current&&<g className="geometry-selection"><rect className="geometry-bounds" x={current.position[0]} y={-current.part.diameter/2} width={Math.max(current.part.length,1)} height={Math.max(current.part.diameter,1)}/>{handle('length',current.position[0]+current.part.length,0)}{handle('diameter',current.position[0]+current.part.length/2,-current.part.diameter/2)}<text className="geometry-readout" x={current.position[0]+current.part.length/2} y={current.part.diameter/2+unit*24} fontSize={unit*12} textAnchor="middle">{current.part.length} × {current.part.diameter} mm · X {current.part.offset}</text></g>}
 </svg>
}
