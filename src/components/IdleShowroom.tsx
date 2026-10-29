import {lazy,Suspense,useEffect,useRef,useState} from 'react';
import type {Project} from '../domain/project';
import {isStructural} from '../domain/componentCatalog';
import './idle-showroom.css';
const RocketView3D=lazy(()=>import('./RocketView3D'));
export const SHOWROOM_IDLE_MS=15000;
export default function IdleShowroom({project,enabled=true}:{project:Project;enabled?:boolean}){
 const [active,setActive]=useState(false),close=useRef(()=>{});
 const physical=project.parts.some(p=>!isStructural(p.kind));
 useEffect(()=>{
  if(!enabled||!physical){setActive(false);return}
  let timer:ReturnType<typeof setTimeout>,showing=false,consumeUntil=0,ownsFullscreen=false,previous:HTMLElement|null=null;
  const main=document.querySelector<HTMLElement>('.app>main');
  function stop(){showing=false;setActive(false);if(main)main.inert=false;if(previous?.isConnected)previous.focus({preventScroll:true});previous=null;if(ownsFullscreen&&document.fullscreenElement)void document.exitFullscreen().catch(()=>{});ownsFullscreen=false}
  function start(){if(showing)return;if(document.hidden||!document.hasFocus()||document.querySelector('dialog[open]')||!document.querySelector('.startup-ready')){arm();return}showing=true;previous=document.activeElement as HTMLElement;if(main)main.inert=true;setActive(true)}
  function arm(){clearTimeout(timer);if(!showing&&!document.hidden&&document.hasFocus())timer=setTimeout(start,SHOWROOM_IDLE_MS)}
  function activity(event:Event){if(showing){event.preventDefault();event.stopImmediatePropagation();consumeUntil=performance.now()+250;stop()}else if(performance.now()<consumeUntil&&['click','pointerup','keyup'].includes(event.type)){event.preventDefault();event.stopImmediatePropagation()}arm()}
  function visibility(){clearTimeout(timer);if(showing)stop();if(!document.hidden)arm()}
  function manual(){clearTimeout(timer);start();if(showing&&!document.fullscreenElement){void document.documentElement.requestFullscreen().then(()=>{if(showing)ownsFullscreen=true;else void document.exitFullscreen().catch(()=>{})}).catch(()=>{})}}
  close.current=()=>{stop();arm()};
  const events=['pointermove','pointerdown','pointerup','click','keydown','keyup','wheel','touchstart'];for(const event of events)window.addEventListener(event,activity,{capture:true,passive:false});
  document.addEventListener('visibilitychange',visibility);window.addEventListener('blur',visibility);window.addEventListener('focus',arm);window.addEventListener('starlab:showroom',manual);arm();
  return()=>{showing=false;clearTimeout(timer);for(const event of events)window.removeEventListener(event,activity,true);document.removeEventListener('visibilitychange',visibility);window.removeEventListener('blur',visibility);window.removeEventListener('focus',arm);window.removeEventListener('starlab:showroom',manual);if(main)main.inert=false;if(ownsFullscreen&&document.fullscreenElement)void document.exitFullscreen().catch(()=>{})};
 },[enabled,physical]);
 if(!active)return null;
 return <div className="idle-showroom" role="region" aria-label="STARLAB showroom"><Suspense fallback={<div className="showroom-loading"/>}><RocketView3D project={project} selected="" selectedIds={[]} focus={project.parts.find(p=>p.kind==='rocket')!.id} onSelect={()=>{}} zoom={1} presentation onBack={()=>close.current()}/></Suspense><div className="showroom-vignette"/></div>;
}
