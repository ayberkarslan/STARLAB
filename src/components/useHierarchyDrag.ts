import {useEffect,useRef,useState,type PointerEvent as ReactPointerEvent} from 'react';
import type {Project} from '../domain/project';
import {moveError,type MoveTarget} from '../domain/move';
export interface DragPreview {source:string;x:number;y:number;target:MoveTarget|null;error:string|null}
export default function useHierarchyDrag(project:Project,onMove:(source:string,target:MoveTarget)=>void){
 const current=useRef({project,onMove});current.current={project,onMove};
 const gesture=useRef<{source:string;x:number;y:number;pointerId:number;moved:boolean}|null>(null);
 const suppressClick=useRef(false);const [preview,setPreview]=useState<DragPreview|null>(null);
 useEffect(()=>{
  let pointer:{x:number;y:number}|null=null;let frame=0;
  const targetAt=(x:number,y:number):MoveTarget|null=>{const element=document.elementFromPoint(x,y)?.closest<HTMLElement>('[data-hierarchy-id]');if(!element)return null;const rect=element.getBoundingClientRect();const fraction=(y-rect.top)/rect.height;return {id:element.dataset.hierarchyId!,placement:element.dataset.dropMode==='inside'?'inside':fraction<.25?'before':fraction>.75?'after':'inside'};};
  const cancel=()=>{gesture.current=null;pointer=null;setPreview(null);cancelAnimationFrame(frame);frame=0};
  const scroll=()=>{if(!pointer||!gesture.current?.moved)return;const container=document.elementFromPoint(pointer.x,pointer.y)?.closest<HTMLElement>('[data-hierarchy-scroll]');if(container){const r=container.getBoundingClientRect();container.scrollTop+=pointer.y<r.top+35?-12:pointer.y>r.bottom-35?12:0;container.scrollLeft+=pointer.x<r.left+35?-12:pointer.x>r.right-35?12:0;const target=targetAt(pointer.x,pointer.y);const error=target?moveError(current.current.project,gesture.current.source,target):'Bir hedef seç.';setPreview(old=>old&&(old.target?.id!==target?.id||old.target?.placement!==target?.placement||old.error!==error)?{...old,target,error}:old);}frame=requestAnimationFrame(scroll)};
  const move=(event:PointerEvent)=>{const g=gesture.current;if(!g||g.pointerId!==event.pointerId)return;if(!g.moved&&Math.hypot(event.clientX-g.x,event.clientY-g.y)<7)return;g.moved=true;suppressClick.current=true;pointer={x:event.clientX,y:event.clientY};if(!frame)frame=requestAnimationFrame(scroll);const target=targetAt(event.clientX,event.clientY);setPreview({source:g.source,x:event.clientX,y:event.clientY,target,error:target?moveError(current.current.project,g.source,target):'Bir hedef seç.'});if(event.cancelable)event.preventDefault()};
  const finish=(event:PointerEvent)=>{const g=gesture.current;if(!g||g.pointerId!==event.pointerId)return;const target=targetAt(event.clientX,event.clientY);if(g.moved&&target&&!moveError(current.current.project,g.source,target))current.current.onMove(g.source,target);cancel();setTimeout(()=>{suppressClick.current=false},0)};
  const escape=(event:KeyboardEvent)=>{if(event.key==='Escape'&&gesture.current){event.preventDefault();cancel()}};
  document.addEventListener('pointermove',move,{passive:false});document.addEventListener('pointerup',finish);document.addEventListener('pointercancel',cancel);document.addEventListener('keydown',escape);window.addEventListener('blur',cancel);
  return()=>{cancel();document.removeEventListener('pointermove',move);document.removeEventListener('pointerup',finish);document.removeEventListener('pointercancel',cancel);document.removeEventListener('keydown',escape);window.removeEventListener('blur',cancel)};
 },[]);
 return {preview,suppressClick,start:(event:ReactPointerEvent,source:string)=>{if(event.button!==0||current.current.project.parts.find(p=>p.id===source)?.kind==='rocket')return;suppressClick.current=false;gesture.current={source,x:event.clientX,y:event.clientY,pointerId:event.pointerId,moved:false}},cancel:()=>{gesture.current=null;setPreview(null)}};
}
