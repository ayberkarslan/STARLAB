import React,{useMemo,useEffect,useRef,useState} from 'react';
import {Canvas,useThree,useFrame,type ThreeEvent} from '@react-three/fiber';
import {PerspectiveCamera,Vector3} from 'three';
import {OrbitControls} from 'three/examples/jsm/controls/OrbitControls.js';
import {resolveParts,scopeParts,boundsOf,type ResolvedPart,type Bounds} from '../domain/geometry';
import {gridGeometry} from './geometry3d';
import {geometryResource,retainGeometry,releaseGeometry,sceneMaterials,type SceneMaterials} from './resources3d';
import type {ViewProps} from './ViewProps';
import {geometrySelection} from '../domain/selectionGeometry';
import {isFin} from '../domain/componentCatalog';

function PartMesh({node,bodyDiameter,selected,onSelect,materials}:{node:ResolvedPart;bodyDiameter:number;selected:boolean;onSelect:(id:string,additive?:boolean)=>void;materials:SceneMaterials}){
 const {kind,length,diameter}=node.part;
 const resource=useMemo(()=>geometryResource(kind,length,diameter,bodyDiameter,node.part.geometry),[kind,length,diameter,bodyDiameter,node.part.geometry]);
 const {geometry,edges}=resource;
 useEffect(()=>{retainGeometry(resource);return()=>releaseGeometry(resource)},[resource]);
 function pick(event:ThreeEvent<MouseEvent>){if(event.delta>5)return;event.stopPropagation();onSelect(node.part.id,event.shiftKey);}
 const count=isFin(kind)?node.part.geometry?.finCount??3:1;
 return <>{Array.from({length:count},(_,i)=><group key={i} rotation={[i*Math.PI*2/count,0,0]}>
 <mesh geometry={geometry} material={materials.surface} dispose={null} onClick={pick}/>
 <lineSegments geometry={edges} material={materials.edge} dispose={null}/>
 {selected&&<mesh geometry={geometry} material={materials.selected} dispose={null} raycast={()=>{}}/>}
 </group>)}</>;
}
function CameraRig({bounds,focus,zoom}:{bounds:Bounds;focus:string;zoom:number}){
 const {camera,gl,invalidate,size}=useThree();const controlRef=useRef<OrbitControls|null>(null);
 const [a,b,c]=bounds.min,[d,e,f]=bounds.max;
 useEffect(()=>{const controls=new OrbitControls(camera,gl.domElement);controlRef.current=controls;const changed=()=>invalidate();controls.enableDamping=true;controls.dampingFactor=.12;controls.addEventListener('change',changed);return()=>{controls.removeEventListener('change',changed);controls.dispose();controlRef.current=null}},[camera,gl,invalidate]);
 useEffect(()=>{const controls=controlRef.current;if(!controls)return;const center=new Vector3((a+d)/2000,(b+e)/2000,(c+f)/2000);const radius=Math.max(new Vector3(d-a,e-b,f-c).length()/2000,.05);const perspective=camera as PerspectiveCamera;const vertical=perspective.fov*Math.PI/360;const angle=Math.min(vertical,Math.atan(Math.tan(vertical)*size.width/Math.max(size.height,1)));const distance=radius/Math.sin(angle)*1.18/zoom;
 controls.target.copy(center);camera.position.copy(center).add(new Vector3(.4,.55,1).normalize().multiplyScalar(distance));perspective.near=Math.max(radius/1000,.0001);perspective.far=Math.max(distance+radius*20,100);perspective.updateProjectionMatrix();controls.update();invalidate();
 },[a,b,c,d,e,f,focus,zoom,size.width,size.height,camera,gl,invalidate]);
 useFrame(()=>controlRef.current?.update());return null;
}
function WorldGrid({floor}:{floor:number}){const geometry=useMemo(gridGeometry,[]);useEffect(()=>()=>geometry.dispose(),[geometry]);return <lineSegments geometry={geometry} position={[0,floor,0]} raycast={()=>{}}><lineBasicMaterial color="white" transparent opacity={.07}/></lineSegments>}
function StudioLights({bounds}:{bounds:Bounds}){
 const target=useMemo(()=>new Vector3(...bounds.min).add(new Vector3(...bounds.max)).multiplyScalar(.0005),[...bounds.min,...bounds.max]);
 // Keep the light directions consistent when isolating a small subsystem.
 const span=Math.max(new Vector3(...bounds.max).sub(new Vector3(...bounds.min)).length()*.001,1);
 return <><hemisphereLight args={['white','white',.9]}/>{[
  {offset:[-.5,1,1],intensity:3.5},
  {offset:[.6,.3,-1],intensity:2},
  {offset:[0,-.5,.6],intensity:.7},
 ].map(({offset,intensity},index)=><directionalLight key={index} color="white" intensity={intensity} position={[target.x+offset[0]*span,target.y+offset[1]*span,target.z+offset[2]*span]} onUpdate={light=>{light.target.position.copy(target);light.target.updateMatrixWorld()}}/>)}</>;
}
function Scene(props:ViewProps){
 const materials=useMemo(sceneMaterials,[]);useEffect(()=>()=>{Object.values(materials).forEach(material=>material.dispose())},[materials]);
 const resolved=useMemo(()=>resolveParts(props.project),[props.project]);const scope=scopeParts(resolved,props.focus);const visible=new Set(scope.map(n=>n.part.id));const bounds=boundsOf(scope);const nodes=new Map(resolved.map(n=>[n.part.id,n]));
 const active=new Set(geometrySelection(props.project,props.selectedIds??[props.selected]).nodes.map(n=>n.part.id));
 const children=new Map<string|null,ResolvedPart[]>();for(const node of resolved){const list=children.get(node.parentId)||[];list.push(node);children.set(node.parentId,list);}
 const branch=(node:ResolvedPart):React.ReactNode=><group key={node.part.id} position={node.localPosition.map(v=>v*.001) as [number,number,number]}>{node.physical&&visible.has(node.part.id)&&<PartMesh node={node} materials={materials} bodyDiameter={nodes.get(node.parentId||'')?.part.diameter||140} selected={active.has(node.part.id)} onSelect={props.onSelect}/>} {(children.get(node.part.id)||[]).map(branch)}</group>;
 return <><CameraRig bounds={bounds} focus={props.focus} zoom={props.zoom}/><StudioLights bounds={bounds}/><WorldGrid floor={bounds.min[1]*.001-.04}/>{(children.get(null)||[]).map(branch)}</>;
}
class Boundary extends React.Component<{children:React.ReactNode;onBack:()=>void},{failed:boolean}>{state={failed:false};static getDerivedStateFromError(){return {failed:true}}render(){return this.state.failed?<div className="view-fallback" role="alert"><p>3D görünüm başlatılamadı.</p><button onClick={this.props.onBack}>2D görünüme dön</button></div>:this.props.children}}
export default function RocketView3D(props:ViewProps&{onBack:()=>void}){
 const [lost,setLost]=useState(false);const start=useRef<[number,number]>([0,0]);
 const fail=()=>setLost(true);
 useEffect(()=>{const canvas=document.querySelector('.three-view canvas');canvas?.addEventListener('webglcontextlost',fail);return()=>canvas?.removeEventListener('webglcontextlost',fail)},[]);
 return <Boundary onBack={props.onBack}>{lost?<div className="view-fallback" role="alert"><p>3D bağlantısı kesildi.</p><button onClick={props.onBack}>2D görünüme dön</button></div>:<div className="three-view" onPointerDown={e=>{start.current=[e.clientX,e.clientY];e.currentTarget.closest<HTMLElement>('.drawing')?.focus()}}><Canvas frameloop="demand" dpr={[1,1.5]} gl={{antialias:true,powerPreference:'low-power'}} camera={{fov:40,position:[2,1,3]}} onPointerMissed={e=>{if(Math.hypot(e.clientX-start.current[0],e.clientY-start.current[1])<5)props.onSelect('')}}><color attach="background" args={['black']}/><Scene {...props}/></Canvas></div>}</Boundary>;
}
