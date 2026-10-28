import { useMemo } from 'react';
import { resolveParts,scopeParts,boundsOf,noseProfile,finProfile } from '../domain/geometry';
import type { ViewProps } from './ViewProps';
export default function RocketView({project,selected,focus,onSelect,zoom}:ViewProps){
 const resolved=useMemo(()=>resolveParts(project),[project]);const shown=scopeParts(resolved,focus).filter(n=>n.physical);const bounds=boundsOf(shown);const min=bounds.min[0],max=bounds.max[0];const radius=Math.max(bounds.max[1],100);const height=Math.max(500,radius*4);const width=Math.max(300,max-min)+300;
 return <svg className="rocket-svg" viewBox={`${min-150} ${-height/2} ${width} ${height}`} style={{transform:`scale(${zoom})`}} aria-label="Roket teknik kesiti">
 <defs><pattern id="hatch" width="18" height="18" patternUnits="userSpaceOnUse"><path d="M0 18L18 0" stroke="#fff" strokeWidth="1"/></pattern></defs>
 <line x1={min-150} x2={max+150} y1="0" y2="0" stroke="#fff" strokeWidth="1" strokeDasharray="12 12"/>
 {shown.map(({part:n,position})=>{const x=position[0],d=n.diameter/2;const parent=project.parts.find(p=>p.id===n.parentId);const outline=n.kind==='nose'?noseProfile(n.length,d):finProfile(n.length,d,(parent?.diameter||140)/2);return <g key={n.id} className={`rocket-part ${selected===n.id?'selected':''} ${['motor','parachute','cord'].includes(n.kind)?'internal':''}`} tabIndex={0} role="button" aria-label={n.name} onClick={()=>onSelect(n.id)} onKeyDown={e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();onSelect(n.id)}}}><title>{n.name}</title>{n.kind==='nose'?<polygon points={[...outline.map(([a,b])=>`${x+a},${-b}`),...outline.slice().reverse().map(([a,b])=>`${x+a},${b}`)].join(' ')}/>:n.kind==='fins'?<>{[-1,1].map(sign=><polygon key={sign} points={outline.map(([a,b])=>`${x+a},${b*sign}`).join(' ')}/>)}</>:<rect x={x} y={-d} width={Math.max(n.length,1)} height={Math.max(n.diameter,1)} rx={n.kind==='parachute'?15:0}/>}</g>})}
 <path d={`M${min} ${radius+40}v20 H${max}v-20`} fill="none" stroke="#fff" strokeWidth="2"/><text x={(min+max)/2} y={radius+100} textAnchor="middle" fill="#fff" fontSize="24">{Math.round(max-min)} mm</text>
 </svg>
}
