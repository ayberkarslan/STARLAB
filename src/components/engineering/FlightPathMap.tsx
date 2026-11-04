import type {FlightSample} from '../../domain/flight';

export default function FlightPathMap({samples}:{samples:FlightSample[]}){
 const points=samples.filter((item):item is FlightSample&{north:number;east:number}=>Number.isFinite(item.north)&&Number.isFinite(item.east));
 if(points.length<2)return <div className="engineering-empty">Kuzey ve doğu konumu yok</div>;
 const north=points.map(item=>item.north),east=points.map(item=>item.east);
 const minN=Math.min(0,...north),maxN=Math.max(0,...north),minE=Math.min(0,...east),maxE=Math.max(0,...east);
 const rangeN=Math.max(1,maxN-minN),rangeE=Math.max(1,maxE-minE),scale=Math.min(900/rangeE,350/rangeN);
 const x=(value:number)=>50+(value-minE)*scale+(900-rangeE*scale)/2;
 const y=(value:number)=>390-(value-minN)*scale-(350-rangeN*scale)/2;
 const path=points.map((item,index)=>`${index?'L':'M'}${x(item.east).toFixed(2)} ${y(item.north).toFixed(2)}`).join(' ');
 const start=points[0],end=points.at(-1)!;
 return <div className="flight-path"><svg viewBox="0 0 1000 430" role="img" aria-label="Doğu ve kuzey eksenlerinde uçuşun üstten yörüngesi">
  <path className="flight-path-ghost" d={path}/><path className="flight-path-progress" d={path}/>
  <circle className="flight-path-rail-exit" cx={x(start.east)} cy={y(start.north)} r="5"/>
  <circle className="flight-path-event" cx={x(end.east)} cy={y(end.north)} r="5"/>
  <text x="24" y="28">Kuzey ↑</text><text x="976" y="414" textAnchor="end">Doğu →</text>
 </svg></div>;
}
