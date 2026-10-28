import {useEffect,useRef,useState,type CSSProperties,type ReactNode} from 'react';
import './startup.css';
import {waitForStartup} from '../domain/startup';

// Randomise once per launch; CSS owns the twinkle animation, not a render loop.
const stars=Array.from({length:96},()=>({x:Math.random()*100,y:Math.random()*100,size:Math.random()>.9?2:1,delay:-Math.random()*6,duration:2.2+Math.random()*3.8,brightness:.3+Math.random()*.7}));

export default function Startup({children}:{children:ReactNode}){
 const [phase,setPhase]=useState<'showing'|'fading'|'ready'>('showing');
 const workspace=useRef<HTMLDivElement>(null),logo=useRef<HTMLImageElement>(null);
 useEffect(()=>{
  const controller=new AbortController();let firstFrame=0,secondFrame=0;
  const painted=new Promise<void>(resolve=>{firstFrame=requestAnimationFrame(()=>{secondFrame=requestAnimationFrame(()=>resolve())})});
  const font=Promise.resolve().then(()=>document.fonts?.load('16px "YRT Michroma"'));
  const image=Promise.resolve().then(()=>logo.current?.decode());
  void waitForStartup([painted,font,image],{signal:controller.signal}).then(()=>{if(!controller.signal.aborted)setPhase('fading')}).catch(()=>{});
  return()=>{controller.abort();cancelAnimationFrame(firstFrame);cancelAnimationFrame(secondFrame)};
 },[]);
 useEffect(()=>{if(phase!=='fading')return;const timer=setTimeout(()=>setPhase('ready'),650);return()=>clearTimeout(timer)},[phase]);
 useEffect(()=>{const blocked=phase!=='ready';if(workspace.current)workspace.current.inert=blocked;if(!blocked)return;const stop=(event:KeyboardEvent)=>{if(!event.ctrlKey&&!event.metaKey&&!event.altKey){event.preventDefault();event.stopImmediatePropagation()}};window.addEventListener('keydown',stop,true);return()=>window.removeEventListener('keydown',stop,true)},[phase]);
 return <div className={`startup-shell startup-${phase}`}>
 <div ref={workspace} className="startup-workspace" aria-hidden={phase!=='ready'?true:undefined}>{children}</div>
 {phase!=='ready'&&<div className="startup-screen" role="status" aria-label="STARLAB hazırlanıyor" aria-busy="true">
 <div className="startup-stars" aria-hidden="true">{stars.map((star,i)=><span key={i} style={{left:`${star.x}%`,top:`${star.y}%`,width:star.size,height:star.size,'--star-delay':`${star.delay}s`,'--star-duration':`${star.duration}s`,'--star-brightness':star.brightness} as CSSProperties}/>)}</div>
 <div className="startup-brand"><svg className="startup-orbit" viewBox="0 0 240 240" aria-hidden="true"><circle className="orbit-track" cx="120" cy="120" r="112"/><g className="orbit-rotor"><circle className="orbit-trail" cx="120" cy="120" r="112"/><path className="orbit-star" d="M120 2 L121.6 6.4 L126 8 L121.6 9.6 L120 14 L118.4 9.6 L114 8 L118.4 6.4Z"/></g></svg><img ref={logo} src="/brand/starlab.png" alt="" draggable="false" onError={event=>{event.currentTarget.style.visibility='hidden'}}/><span aria-hidden="true">STARLAB</span></div>
 </div>}
 </div>;
}
