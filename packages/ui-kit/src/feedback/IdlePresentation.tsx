import {useEffect,useRef,useState,type ReactNode} from 'react';
export interface IdlePresentationProps {enabled?:boolean;idleMs?:number;eventName?:string;persistentEventName?:string;workspaceSelector?:string;canStart?:()=>boolean;label?:string;className?:string;children:(close:()=>void)=>ReactNode}
/** Owns activity/fullscreen lifecycle; the caller supplies its own scene or content. */
export function IdlePresentation({enabled=true,idleMs=15000,eventName='stardrop:presentation',persistentEventName,workspaceSelector='.sd-desktop-content',canStart,label='Sunum',className='',children}:IdlePresentationProps){
 const [active,setActive]=useState(false),close=useRef(()=>{}),allowed=useRef(canStart);allowed.current=canStart;
 useEffect(()=>{
  if(!enabled){setActive(false);return}
  let timer:ReturnType<typeof setTimeout>,showing=false,pinned=false,consumeUntil=0,ownsFullscreen=false,previous:HTMLElement|null=null;
  const main=document.querySelector<HTMLElement>(workspaceSelector),wasInert=main?.inert??false;
  function stop(){showing=false;setActive(false);if(main)main.inert=wasInert;if(previous?.isConnected)previous.focus({preventScroll:true});previous=null;if(ownsFullscreen&&document.fullscreenElement)void document.exitFullscreen().catch(()=>{});ownsFullscreen=false}
  function start(){if(showing)return;if(document.hidden||!document.hasFocus()||document.querySelector('dialog[open]')||allowed.current?.()===false){arm();return}showing=true;previous=document.activeElement as HTMLElement;if(main)main.inert=true;setActive(true)}
  function arm(){clearTimeout(timer);if(!showing&&!document.hidden&&document.hasFocus())timer=setTimeout(start,idleMs)}
  function activity(event:Event){if(event.type==='keydown'&&persistentEventName&&(event as KeyboardEvent).altKey&&!((event as KeyboardEvent).ctrlKey||(event as KeyboardEvent).metaKey||(event as KeyboardEvent).repeat)&&(event as KeyboardEvent).key.toLowerCase()==='s'){event.preventDefault();event.stopImmediatePropagation();togglePinned();return}if(showing){event.preventDefault();event.stopImmediatePropagation();if(pinned)return;consumeUntil=performance.now()+250;stop()}else if(performance.now()<consumeUntil&&['click','pointerup','keyup'].includes(event.type)){event.preventDefault();event.stopImmediatePropagation()}arm()}
  function visibility(){if(pinned)return;clearTimeout(timer);if(showing)stop();if(!document.hidden)arm()}
  function manual(){clearTimeout(timer);start();if(showing&&!document.fullscreenElement){void document.documentElement.requestFullscreen().then(()=>{if(showing)ownsFullscreen=true;else void document.exitFullscreen().catch(()=>{})}).catch(()=>{})}}
  function togglePinned(){if(pinned){pinned=false;stop();arm()}else{pinned=true;manual();if(!showing)pinned=false}}
  close.current=()=>{if(!pinned){stop();arm()}};
  const events=['pointermove','pointerdown','pointerup','click','keydown','keyup','wheel','touchstart'];for(const event of events)window.addEventListener(event,activity,{capture:true,passive:false});
  document.addEventListener('visibilitychange',visibility);window.addEventListener('blur',visibility);window.addEventListener('focus',arm);window.addEventListener(eventName,manual);if(persistentEventName)window.addEventListener(persistentEventName,togglePinned);arm();
  return()=>{showing=false;clearTimeout(timer);for(const event of events)window.removeEventListener(event,activity,true);document.removeEventListener('visibilitychange',visibility);window.removeEventListener('blur',visibility);window.removeEventListener('focus',arm);window.removeEventListener(eventName,manual);if(persistentEventName)window.removeEventListener(persistentEventName,togglePinned);if(main)main.inert=wasInert;if(ownsFullscreen&&document.fullscreenElement)void document.exitFullscreen().catch(()=>{})};
 },[enabled,idleMs,eventName,persistentEventName,workspaceSelector]);
 return active?<div className={`sd-idle-presentation ${className}`} role="region" aria-label={label}>{children(()=>close.current())}</div>:null;
}
