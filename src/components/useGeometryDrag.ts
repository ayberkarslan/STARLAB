import {useEffect,useRef,useState,type PointerEvent as ReactPointerEvent,type RefObject} from 'react';
import type {Part,Project} from '../domain/project';
import {geometryPatch,minimumDiameter,type GeometryEdit,type GeometryPatch} from '../domain/geometryEdit';
import {snapCandidates,snapGeometry,type GeometrySnap} from '../domain/geometrySnap';

interface Gesture {pointer:number;part:Part;mode:GeometryEdit;inverse:DOMMatrix;start:DOMPoint;clientX:number;clientY:number;patch:GeometryPatch;changed:boolean;candidates:GeometrySnap[];snap:GeometrySnap|null;diameterFloor:number}
export function useGeometryDrag(svg:RefObject<SVGSVGElement>,project:Project,focus:string,onEdit:((id:string,patch:GeometryPatch)=>void)|undefined,onPreview:((part:Part|null)=>void)|undefined,snapEnabled=true){
 const gesture=useRef<Gesture|null>(null),callbacks=useRef({onEdit,onPreview,snapEnabled});callbacks.current={onEdit,onPreview,snapEnabled};
 const [draft,setDraft]=useState<{id:string;patch:GeometryPatch;snap:GeometrySnap|null}|null>(null);
 const finish=(commit:boolean)=>{
  const g=gesture.current;if(!g)return;gesture.current=null;setDraft(null);callbacks.current.onPreview?.(null);
  if(svg.current?.hasPointerCapture(g.pointer))svg.current.releasePointerCapture(g.pointer);
  if(commit&&g.changed)callbacks.current.onEdit?.(g.part.id,g.patch);
 };
 useEffect(()=>{
  const cancel=()=>finish(false);
  const key=(event:KeyboardEvent)=>{if(!gesture.current)return;
   if(event.key==='Escape'||event.key==='Delete'||(event.ctrlKey||event.metaKey)&&event.key.toLowerCase()==='z'){event.preventDefault();event.stopImmediatePropagation();cancel();}
   else if(event.key==='Tab')cancel();
  };
  window.addEventListener('keydown',key,true);window.addEventListener('blur',cancel);window.addEventListener('resize',cancel);
  return()=>{window.removeEventListener('keydown',key,true);window.removeEventListener('blur',cancel);window.removeEventListener('resize',cancel);cancel()};
 },[]);
 useEffect(()=>{finish(false)},[project,focus]);
 const start=(event:ReactPointerEvent<SVGGElement>,part:Part,mode:GeometryEdit)=>{
  if(!callbacks.current.onEdit||event.button!==0||gesture.current)return;
  const root=svg.current,inverse=root?.getScreenCTM()?.inverse();if(!root||!inverse)return;
  event.preventDefault();event.stopPropagation();event.currentTarget.focus({preventScroll:true});
  const diameterFloor=minimumDiameter(project,part);const candidates=snapCandidates(project,part,mode).filter(c=>c.key!=='diameter'||c.value>=diameterFloor);
  gesture.current={pointer:event.pointerId,part,mode,inverse,start:new DOMPoint(event.clientX,event.clientY).matrixTransform(inverse),clientX:event.clientX,clientY:event.clientY,patch:{},changed:false,candidates,snap:null,diameterFloor};
  root.setPointerCapture(event.pointerId);setDraft({id:part.id,patch:{},snap:null});
 };
 const move=(event:ReactPointerEvent<SVGSVGElement>)=>{
  const g=gesture.current;if(!g||event.pointerId!==g.pointer)return;
  const distance=g.mode==='diameter'?event.clientY-g.clientY:event.clientX-g.clientX;
  if(!g.changed&&Math.abs(distance)<3)return;
  const point=new DOMPoint(event.clientX,event.clientY).matrixTransform(g.inverse);
  const delta=g.mode==='diameter'?-2*(point.y-g.start.y):point.x-g.start.x;
  g.patch=geometryPatch(g.part,g.mode,delta,event.shiftKey?10:event.altKey?0.1:1,g.diameterFloor);g.changed=true;
  const aligned=callbacks.current.snapEnabled&&!event.altKey?snapGeometry(g.patch,g.candidates,Math.hypot(g.inverse.a,g.inverse.b),g.snap):{patch:g.patch,snap:null};
  g.patch=aligned.patch;g.snap=aligned.snap;
  setDraft({id:g.part.id,patch:g.patch,snap:g.snap});callbacks.current.onPreview?.({...g.part,...g.patch});
 };
 return {draft,start,move,finish};
}
