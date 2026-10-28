import {isStructural} from '../domain/componentCatalog';
import {useEffect,useRef,useState,type PointerEvent as ReactPointerEvent,type RefObject} from 'react';
import type {Part,Project} from '../domain/project';
import {geometryPatch,minimumDiameter,type GeometryEdit,type GeometryPatch} from '../domain/geometryEdit';
import {snapCandidates,snapGeometry,type GeometrySnap} from '../domain/geometrySnap';
import {transformSelection,geometrySelection} from '../domain/selectionGeometry';
import {selectionSnapCandidates} from '../domain/selectionSnap';

interface Batch {ids:string[];onEdit:(project:Project)=>void;onPreview:(parts:Part[]|null)=>void}
interface Gesture {pointer:number;part:Part;mode:GeometryEdit;inverse:DOMMatrix;start:DOMPoint;clientX:number;clientY:number;patch:GeometryPatch;changed:boolean;candidates:GeometrySnap[];snap:GeometrySnap|null;diameterFloor:number;batchIds?:string[];base:Project;next?:Project}
export function useGeometryDrag(svg:RefObject<SVGSVGElement>,project:Project,focus:string,onEdit:((id:string,patch:GeometryPatch)=>void)|undefined,onPreview:((part:Part|null)=>void)|undefined,snapEnabled=true,batch?:Batch){
 const gesture=useRef<Gesture|null>(null),callbacks=useRef({onEdit,onPreview,snapEnabled,batch});callbacks.current={onEdit,onPreview,snapEnabled,batch};
 const [draft,setDraft]=useState<{id:string;patch:GeometryPatch;snap:GeometrySnap|null;project?:Project}|null>(null);
 const finish=(commit:boolean)=>{
  const g=gesture.current;if(!g)return;gesture.current=null;setDraft(null);callbacks.current.onPreview?.(null);callbacks.current.batch?.onPreview(null);
  if(svg.current?.hasPointerCapture(g.pointer))svg.current.releasePointerCapture(g.pointer);
  if(commit&&g.changed){if(g.batchIds&&g.next)callbacks.current.batch?.onEdit(g.next);else callbacks.current.onEdit?.(g.part.id,g.patch);}
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
 const start=(event:ReactPointerEvent<SVGElement>,part:Part,mode:GeometryEdit)=>{
  if(!callbacks.current.onEdit&&!callbacks.current.batch||event.button!==0||gesture.current)return;
  const root=svg.current,inverse=root?.getScreenCTM()?.inverse();if(!root||!inverse)return;
  event.preventDefault();event.stopPropagation();event.currentTarget.focus({preventScroll:true});
  const diameterFloor=minimumDiameter(project,part);const candidates=snapCandidates(project,part,mode).filter(c=>c.key!=='diameter'||c.value>=diameterFloor);
  const ids=batch?.ids.includes(part.id)?batch.ids:[part.id];const batchIds=batch&&(ids.length>1||(isStructural(part.kind)&&part.kind!=='rocket'))?ids:undefined;
  const selected=batchIds?geometrySelection(project,batchIds):undefined;
  const reference=selected?{...part,length:selected.bounds.max[0]-selected.bounds.min[0],diameter:Math.max(...selected.nodes.map(n=>n.part.diameter)),offset:0}:part;
  gesture.current={pointer:event.pointerId,part:reference,mode,inverse,start:new DOMPoint(event.clientX,event.clientY).matrixTransform(inverse),clientX:event.clientX,clientY:event.clientY,patch:{},changed:false,candidates:batchIds?selectionSnapCandidates(project,batchIds,mode):candidates,snap:null,diameterFloor,batchIds,base:project};
  root.setPointerCapture(event.pointerId);setDraft({id:part.id,patch:{},snap:null});
 };
 const move=(event:ReactPointerEvent<SVGSVGElement>)=>{
  const g=gesture.current;if(!g||event.pointerId!==g.pointer)return;
  const distance=g.mode==='diameter'?event.clientY-g.clientY:event.clientX-g.clientX;
  if(!g.changed&&Math.abs(distance)<3)return;
  const point=new DOMPoint(event.clientX,event.clientY).matrixTransform(g.inverse);
  const delta=g.mode==='diameter'?-2*(point.y-g.start.y):point.x-g.start.x;
  if(g.batchIds){
   const step=event.shiftKey?10:event.altKey?0.1:1,key=g.mode==='move'?'offset':g.mode;
   const patch=geometryPatch(g.part,g.mode,delta,step);
   const aligned=callbacks.current.snapEnabled&&!event.altKey?snapGeometry(patch,g.candidates,Math.hypot(g.inverse.a,g.inverse.b),g.snap):{patch,snap:null};
   g.next=transformSelection(g.base,g.batchIds,g.mode,aligned.patch[key]!-g.part[key],aligned.snap?0.001:step);g.changed=g.next!==g.base;g.snap=aligned.snap;
   const result=geometrySelection(g.next,g.batchIds);const actual=g.mode==='move'?result.bounds.min[0]-geometrySelection(g.base,g.batchIds).bounds.min[0]:g.mode==='length'?result.bounds.max[0]-result.bounds.min[0]:Math.max(...result.nodes.map(n=>n.part.diameter));
   if(g.snap&&Math.abs(actual-g.snap.value)>.01)g.snap=null;
   setDraft({id:g.part.id,patch:{},snap:g.snap,project:g.next});callbacks.current.batch?.onPreview(g.next.parts);return;
  }
  g.patch=geometryPatch(g.part,g.mode,delta,event.shiftKey?10:event.altKey?0.1:1,g.diameterFloor);g.changed=true;
  const aligned=callbacks.current.snapEnabled&&!event.altKey?snapGeometry(g.patch,g.candidates,Math.hypot(g.inverse.a,g.inverse.b),g.snap):{patch:g.patch,snap:null};
  g.patch=aligned.patch;g.snap=aligned.snap;
  setDraft({id:g.part.id,patch:g.patch,snap:g.snap});callbacks.current.onPreview?.({...g.part,...g.patch});
 };
 return {draft,start,move,finish};
}
