import {useEffect,useRef,useState,type CSSProperties,type ReactNode} from 'react';
import './startup.css';

// Randomise once per launch; CSS owns the twinkle animation, not a render loop.
const stars=Array.from({length:96},()=>({x:Math.random()*100,y:Math.random()*100,size:Math.random()>.9?2:1,delay:-Math.random()*6,duration:2.2+Math.random()*3.8,brightness:.3+Math.random()*.7}));

export default function Startup({children}:{children:ReactNode}){
 const [phase,setPhase]=useState<'showing'|'fading'|'ready'>('showing');
 const workspace=useRef<HTMLDivElement>(null);
 useEffect(()=>{const fade=setTimeout(()=>setPhase('fading'),2000);const finish=setTimeout(()=>setPhase('ready'),2650);return()=>{clearTimeout(fade);clearTimeout(finish)}},[]);
 useEffect(()=>{const blocked=phase!=='ready';if(workspace.current)workspace.current.inert=blocked;if(!blocked)return;const stop=(event:KeyboardEvent)=>{if(!event.ctrlKey&&!event.metaKey&&!event.altKey){event.preventDefault();event.stopImmediatePropagation()}};window.addEventListener('keydown',stop,true);return()=>window.removeEventListener('keydown',stop,true)},[phase]);
 return <div className={`startup-shell startup-${phase}`}>
 <div ref={workspace} className="startup-workspace" aria-hidden={phase!=='ready'?true:undefined}>{children}</div>
 {phase!=='ready'&&<div className="startup-screen" role="status" aria-label="STARLAB açılıyor">
 <div className="startup-stars" aria-hidden="true">{stars.map((star,i)=><span key={i} style={{left:`${star.x}%`,top:`${star.y}%`,width:star.size,height:star.size,'--star-delay':`${star.delay}s`,'--star-duration':`${star.duration}s`,'--star-brightness':star.brightness} as CSSProperties}/>)}</div>
 <div className="startup-brand"><img src="/brand/starlab.png" alt="" draggable="false"/><span aria-hidden="true">STARLAB</span></div>
 </div>}
 </div>;
}
