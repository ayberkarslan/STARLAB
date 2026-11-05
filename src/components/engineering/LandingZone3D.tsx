import React,{useEffect,useMemo,useRef,useState} from 'react';
import {Canvas,useFrame,useThree} from '@react-three/fiber';
import {BufferAttribute,BufferGeometry,DoubleSide,PerspectiveCamera,Vector3} from 'three';
import {OrbitControls} from 'three/examples/jsm/controls/OrbitControls.js';
import type {LandingZoneResult} from '../../domain/landingZone';

function lineGeometry(coords:[number,number,number][]){
 const geometry=new BufferGeometry();
 const array=new Float32Array(coords.length*3);
 for(let i=0;i<coords.length;i++){
  array[i*3]=coords[i][0];
  array[i*3+1]=coords[i][1];
  array[i*3+2]=coords[i][2];
 }
 geometry.setAttribute('position',new BufferAttribute(array,3));
 return geometry;
}

function ellipseLoopPoints(centerE:number,centerN:number,a:number,b:number,angleDeg:number,y=0.15,segments=96):[number,number,number][]{
 const rad=angleDeg*Math.PI/180,cos=Math.cos(rad),sin=Math.sin(rad);
 const pts:[number,number,number][]=[];
 for(let i=0;i<=segments;i++){
  const t=(i/segments)*Math.PI*2;
  const lx=a*Math.cos(t),ly=b*Math.sin(t);
  const e=centerE+lx*cos-ly*sin;
  const n=centerN+lx*sin+ly*cos;
  pts.push([e,y,-n]);
 }
 return pts;
}

function GroundSceneGrid({radius,centerE,centerN}:{radius:number;centerE:number;centerN:number}){
 const geometry=useMemo(()=>{
  const segments:[number,number,number][]=[];
  const span=Math.max(50,radius*1.35);
  const step=Math.pow(10,Math.floor(Math.log10(span)))/2;
  const cleanStep=span/step>24?step*2:span/step<8?step/2:step;
  const minE=Math.floor((centerE-span)/cleanStep)*cleanStep,maxE=Math.ceil((centerE+span)/cleanStep)*cleanStep;
  const minN=Math.floor((centerN-span)/cleanStep)*cleanStep,maxN=Math.ceil((centerN+span)/cleanStep)*cleanStep;
  for(let e=minE;e<=maxE;e+=cleanStep){segments.push([e,0,-minN],[e,0,-maxN]);}
  for(let n=minN;n<=maxN;n+=cleanStep){segments.push([minE,0,-n],[maxE,0,-n]);}
  return lineGeometry(segments);
 },[radius,centerE,centerN]);
 useEffect(()=>()=>geometry.dispose(),[geometry]);
 return <lineSegments geometry={geometry}><lineBasicMaterial color="white" transparent opacity={0.09}/></lineSegments>;
}

function Polyline3D({points,opacity,color='white'}:{points:[number,number,number][];opacity:number;color?:string}){
 const geometry=useMemo(()=>lineGeometry(points),[points]);
 useEffect(()=>()=>geometry.dispose(),[geometry]);
 return <lineLoop geometry={geometry}><lineBasicMaterial color={color} transparent opacity={opacity}/></lineLoop>;
}

function OpenLine3D({points,opacity}:{points:[number,number,number][];opacity:number}){
 const geometry=useMemo(()=>{
  const segments:[number,number,number][]=[];
  for(let index=1;index<points.length;index++)segments.push(points[index-1],points[index]);
  return lineGeometry(segments);
 },[points]);
 useEffect(()=>()=>geometry.dispose(),[geometry]);
 return <lineSegments geometry={geometry}><lineBasicMaterial color="white" transparent opacity={opacity}/></lineSegments>;
}

function CameraController({target,span,focusKey}:{target:[number,number,number];span:number;focusKey:string}){
 const {camera,gl,invalidate}=useThree();
 const controlsRef=useRef<OrbitControls|null>(null);
 useEffect(()=>{
  const controls=new OrbitControls(camera,gl.domElement);
  controlsRef.current=controls;
  controls.enableDamping=true;
  controls.dampingFactor=0.12;
  const onChange=()=>invalidate();
  controls.addEventListener('change',onChange);
  return()=>{controls.removeEventListener('change',onChange);controls.dispose();controlsRef.current=null;};
 },[camera,gl,invalidate]);
 useEffect(()=>{
  const controls=controlsRef.current;if(!controls)return;
  const center=new Vector3(...target);
  const dist=Math.max(30,span*1.45);
  const offset=new Vector3(0.45,0.68,0.85).normalize().multiplyScalar(dist);
  controls.target.copy(center);
  camera.position.copy(center).add(offset);
  const perspective=camera as PerspectiveCamera;
  perspective.near=Math.max(0.5,dist/2000);
  perspective.far=Math.max(20000,dist*25);
  perspective.updateProjectionMatrix();
  controls.update();
  invalidate();
 },[focusKey,target[0],target[1],target[2],span,camera,invalidate]);
 useFrame(()=>{controlsRef.current?.update();});
 return null;
}

function LandingScene({result,selectedRun,showTrajectories,focusZone,onSelectRun}:{result:LandingZoneResult;selectedRun:number;showTrajectories:boolean;focusZone:boolean;onSelectRun:(index:number)=>void}){
 const {invalidate}=useThree();
 useEffect(()=>{invalidate();},[result,selectedRun,showTrajectories,focusZone,invalidate]);
 const e=result.ellipse;
 const maxApogee=Math.max(20,...result.points.map(p=>p.apogee));
 const maxGround=Math.max(30,e.maxRangeM+e.semiMajor3Sigma);
 const zoneSpan=Math.max(25,e.semiMajor3Sigma*2.4,Math.hypot(e.centerEast-e.nominalEast,e.centerNorth-e.nominalNorth)*2);
 const cameraTarget:[number,number,number]=focusZone
  ?[e.centerEast,0,-e.centerNorth]
  :[e.centerEast*0.5,maxApogee*0.28,-e.centerNorth*0.5];
 const cameraSpan=focusZone?zoneSpan:Math.max(maxGround,maxApogee*0.85);
 const markerScale=Math.max(1.2,cameraSpan*0.014);

 const loop1=useMemo(()=>ellipseLoopPoints(e.centerEast,e.centerNorth,Math.max(0.5,e.semiMajor1Sigma),Math.max(0.5,e.semiMinor1Sigma),e.angleDeg,0.2),[e]);
 const loop2=useMemo(()=>ellipseLoopPoints(e.centerEast,e.centerNorth,Math.max(1,e.semiMajor2Sigma),Math.max(1,e.semiMinor2Sigma),e.angleDeg,0.15),[e]);
 const loop3=useMemo(()=>ellipseLoopPoints(e.centerEast,e.centerNorth,Math.max(1.5,e.semiMajor3Sigma),Math.max(1.5,e.semiMinor3Sigma),e.angleDeg,0.1),[e]);
 const majorAxis=useMemo(()=>{
  const rad=e.angleDeg*Math.PI/180,a=Math.max(2,e.semiMajor3Sigma),b=Math.max(2,e.semiMinor3Sigma);
  const cos=Math.cos(rad),sin=Math.sin(rad);
  return {
   major:[[e.centerEast-a*cos,0.12,-(e.centerNorth-a*sin)],[e.centerEast+a*cos,0.12,-(e.centerNorth+a*sin)]] as [number,number,number][],
   minor:[[e.centerEast+b*sin,0.12,-(e.centerNorth-b*cos)],[e.centerEast-b*sin,0.12,-(e.centerNorth+b*cos)]] as [number,number,number][],
  };
 },[e]);
 const padTower=useMemo(()=>[[0,0,0],[0,Math.max(8,maxApogee*0.05),0]] as [number,number,number][],[maxApogee]);

 return <>
  <CameraController target={cameraTarget} span={cameraSpan} focusKey={`${result.signature}-${focusZone?'zone':'all'}`}/>
  <ambientLight intensity={0.9}/>
  <directionalLight position={[200,500,300]} intensity={1.6} color="white"/>
  <GroundSceneGrid radius={focusZone?zoneSpan:maxGround} centerE={focusZone?e.centerEast:e.centerEast*0.5} centerN={focusZone?e.centerNorth:e.centerNorth*0.5}/>
  <OpenLine3D points={padTower} opacity={0.9}/>
  <mesh position={[0,0.1,0]} rotation={[-Math.PI/2,0,0]}>
   <ringGeometry args={[markerScale*0.7,markerScale*1.2,24]}/>
   <meshBasicMaterial color="white" side={DoubleSide}/>
  </mesh>
  {!focusZone&&(result.bodies??[]).filter(b=>b.ellipse!==e&&b.ellipse.semiMajor1Sigma>0.05).map(b=>{
   const oLoop=ellipseLoopPoints(b.ellipse.centerEast,b.ellipse.centerNorth,Math.max(1,b.ellipse.semiMajor2Sigma),Math.max(1,b.ellipse.semiMinor2Sigma),b.ellipse.angleDeg,0.12);
   const oTraj=b.points.find(p=>p.index===selectedRun)?.trajectory??b.points[0]?.trajectory??[];
   return <group key={`other-3d-${b.id}`}>
    <Polyline3D points={oLoop} opacity={0.45}/>
    {showTrajectories&&oTraj.length>=2&&<OpenLine3D points={oTraj.map(t=>[t.east,t.altitude,-t.north])} opacity={0.38}/>}
   </group>;
  })}
  {e.semiMajor1Sigma>0.05&&<>
   <Polyline3D points={loop3} opacity={0.35}/>
   <Polyline3D points={loop2} opacity={0.6}/>
   <Polyline3D points={loop1} opacity={0.95}/>
   <OpenLine3D points={majorAxis.major} opacity={0.28}/>
   <OpenLine3D points={majorAxis.minor} opacity={0.28}/>
   <mesh position={[e.centerEast,0.05,-e.centerNorth]} rotation={[-Math.PI/2,0,e.angleDeg*Math.PI/180]} scale={[Math.max(1,e.semiMajor3Sigma),Math.max(1,e.semiMinor3Sigma),1]}>
    <circleGeometry args={[1,64]}/>
    <meshBasicMaterial color="white" transparent opacity={0.05} side={DoubleSide} depthWrite={false}/>
   </mesh>
   <mesh position={[e.centerEast,0.08,-e.centerNorth]} rotation={[-Math.PI/2,0,e.angleDeg*Math.PI/180]} scale={[Math.max(0.5,e.semiMajor1Sigma),Math.max(0.5,e.semiMinor1Sigma),1]}>
    <circleGeometry args={[1,64]}/>
    <meshBasicMaterial color="white" transparent opacity={0.12} side={DoubleSide} depthWrite={false}/>
   </mesh>
  </>}
  {showTrajectories&&result.points.map(point=>{
   if(point.trajectory.length<2)return null;
   const active=point.index===selectedRun;
   const opacity=active?1:point.nominal?0.45:0.15;
   const pts=point.trajectory.map(t=>[t.east,t.altitude,-t.north] as [number,number,number]);
   const peak=active?point.trajectory.reduce((best,cur)=>cur.altitude>best.altitude?cur:best,point.trajectory[0]):null;
   return <group key={`traj-${point.index}`}>
    <OpenLine3D points={pts} opacity={opacity}/>
    {peak&&<mesh position={[peak.east,peak.altitude,-peak.north]}>
     <sphereGeometry args={[markerScale*0.75,12,12]}/>
     <meshBasicMaterial color="white"/>
    </mesh>}
   </group>;
  })}
  {result.points.map(point=>{
   const active=point.index===selectedRun;
   const r=active?markerScale*1.15:point.nominal?markerScale*0.95:markerScale*0.65;
   return <group key={`pt-${point.index}`} position={[point.east,0.2,-point.north]}>
    <mesh onClick={ev=>{ev.stopPropagation();onSelectRun(point.index);}}>
     <sphereGeometry args={[r,14,14]}/>
     <meshBasicMaterial color="white" transparent opacity={active||point.nominal?1:0.72}/>
    </mesh>
    {active&&<mesh rotation={[-Math.PI/2,0,0]}>
     <ringGeometry args={[r*1.5,r*2.1,24]}/>
     <meshBasicMaterial color="white" side={DoubleSide}/>
    </mesh>}
   </group>;
  })}
 </>;
}

class SceneBoundary extends React.Component<{children:React.ReactNode;onFallback:()=>void},{failed:boolean}>{
 state={failed:false};
 static getDerivedStateFromError(){return {failed:true};}
 render(){
  return this.state.failed
   ?<div className="view-fallback" role="alert"><p>3D iniş sahnesi başlatılamadı.</p><button onClick={this.props.onFallback}>2D haritaya geç</button></div>
   :this.props.children;
 }
}

export default function LandingZone3D(props:{result:LandingZoneResult;selectedRun:number;showTrajectories:boolean;focusZone:boolean;onSelectRun:(index:number)=>void;onFallback:()=>void}){
 const [lost,setLost]=useState(false);
 const host=useRef<HTMLDivElement>(null);
 useEffect(()=>{
  const canvas=host.current?.querySelector('canvas');
  const onLost=()=>setLost(true);
  canvas?.addEventListener('webglcontextlost',onLost);
  return()=>canvas?.removeEventListener('webglcontextlost',onLost);
 },[]);
 return <SceneBoundary onFallback={props.onFallback}>
  {lost
   ?<div className="view-fallback" role="alert"><p>3D bağlantısı kesildi.</p><button onClick={props.onFallback}>2D haritaya geç</button></div>
   :<div ref={host} className="three-view landing-three-view">
    <Canvas frameloop="demand" dpr={[1,1.5]} gl={{antialias:true,powerPreference:'low-power'}} camera={{fov:42,position:[200,180,260]}}>
     <color attach="background" args={['black']}/>
     <LandingScene {...props}/>
    </Canvas>
   </div>}
 </SceneBoundary>;
}
