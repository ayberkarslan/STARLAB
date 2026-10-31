import {useEffect,useMemo,useRef,useState} from 'react';
import {IconButton} from '@stardrop/ui';
import {Pause,Play,RefreshCw} from '@stardrop/ui/icons';
import type {FlightSample} from '../../domain/flight';

const WIDTH=1000,HEIGHT=430,PAD_X=52,PAD_Y=42;
export function trajectoryGeometry(samples:FlightSample[]){
 const source=samples.filter((sample):sample is FlightSample&{downrange:number}=>Number.isFinite(sample.downrange));
 if(source.length<2)return null;
 const xs=source.map(sample=>sample.downrange),maxY=Math.max(1,...source.map(sample=>sample.altitude)),minX=Math.min(0,...xs),maxX=Math.max(0,...xs);
 const rangeX=Math.max(1,maxX-minX),scale=Math.min((WIDTH-PAD_X*2)/rangeX,(HEIGHT-PAD_Y*2)/maxY);
 const occupied=rangeX*scale,offsetX=PAD_X+(WIDTH-PAD_X*2-occupied)/2;
 const points=source.map(sample=>({x:offsetX+(sample.downrange-minX)*scale,y:HEIGHT-PAD_Y-sample.altitude*scale,sample}));
 return {points,path:points.map((point,index)=>`${index?'L':'M'}${point.x.toFixed(2)} ${point.y.toFixed(2)}`).join(' '),minX,maxX,maxY,groundY:HEIGHT-PAD_Y};
}

export default function FlightPath2D({samples}:{samples:FlightSample[]}){
 const geometry=useMemo(()=>trajectoryGeometry(samples),[samples]),[frame,setFrame]=useState(0),[playing,setPlaying]=useState(true),request=useRef(0),started=useRef(0);
 const last=geometry?.points.length?geometry.points.length-1:0;
 useEffect(()=>{setFrame(0);setPlaying(!!geometry)},[geometry]);
 useEffect(()=>{if(!playing||!geometry)return;const duration=Math.min(12000,Math.max(4000,(samples.at(-1)?.t||0)*350));started.current=performance.now()-frame/last*duration;const tick=(now:number)=>{const next=Math.min(last,Math.floor((now-started.current)/duration*last));setFrame(next);if(next<last)request.current=requestAnimationFrame(tick);else setPlaying(false)};request.current=requestAnimationFrame(tick);return()=>cancelAnimationFrame(request.current)},[playing,geometry,last,samples]);
 if(!geometry)return <div className="engineering-empty">Bu sonuçta 2D konum verisi yok</div>;
 const point=geometry.points[Math.min(frame,last)],previous=geometry.points[Math.max(0,frame-1)],vx=point.sample.horizontalVelocity,vy=point.sample.verticalVelocity,railExit=geometry.points.find(item=>item.sample.railCleared);
 const angle=Number.isFinite(vx)&&Number.isFinite(vy)?-Math.atan2(vy!,vx!)*180/Math.PI:Math.atan2(point.y-previous.y,point.x-previous.x)*180/Math.PI;
 const progress=geometry.points.slice(0,frame+1).map((item,index)=>`${index?'L':'M'}${item.x.toFixed(2)} ${item.y.toFixed(2)}`).join(' ');
 function toggle(){if(frame>=last){setFrame(0);started.current=0;setPlaying(true)}else setPlaying(value=>!value)}
 return <div className="flight-path">
  <svg viewBox={`0 0 ${WIDTH} ${HEIGHT}`} role="img" aria-label="Yatay mesafe ve irtifaya göre 2D uçuş yörüngesi">
   <line className="flight-path-ground" x1="24" x2={WIDTH-24} y1={geometry.groundY} y2={geometry.groundY}/>
   <path className="flight-path-ghost" d={geometry.path}/><path className="flight-path-progress" d={progress}/>
   {railExit&&<circle className="flight-path-rail-exit" cx={railExit.x} cy={railExit.y} r="4"/>}
   <g className="flight-path-rocket" transform={`translate(${point.x} ${point.y}) rotate(${angle})`}><path d="M 13 0 L -8 -6 L -4 0 L -8 6 Z"/></g>
   <text x="24" y="26">{Math.round(geometry.maxY)} m</text><text x={WIDTH-24} y={HEIGHT-14} textAnchor="end">{Math.round(point.sample.downrange!)} m</text>
  </svg>
  <div className="flight-path-control"><IconButton size="sm" variant="ghost" label={playing?'Duraklat':frame>=last?'Baştan oynat':'Oynat'} onClick={toggle}>{playing?<Pause size={15}/>:frame>=last?<RefreshCw size={15}/>:<Play size={15}/>}</IconButton><output>{point.sample.t.toFixed(1)} s</output></div>
 </div>;
}
