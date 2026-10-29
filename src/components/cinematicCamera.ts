import type {Bounds} from '../domain/geometry';
/** Continuous four-shot orbit: hero, nose detail, tail detail, reverse hero. */
export function cinematicPose(bounds:Bounds,aspect:number,seconds:number){
 const size=bounds.max.map((v,i)=>(v-bounds.min[i])/1000),center=bounds.max.map((v,i)=>(v+bounds.min[i])/2000);
 const radius=Math.max(Math.hypot(...size)/2,.03),halfFov=16*Math.PI/180;
 const fit=Math.max(size[0]/2/(Math.tan(halfFov)*Math.max(aspect,.2)),Math.hypot(size[1],size[2])/2/Math.tan(halfFov),.1)*1.2+radius*.1;
 const shot=seconds/12,index=Math.floor(shot)%4,t=shot-Math.floor(shot),s=t*t*(3-2*t);
 const targets=[.5,.12,.84,.5,.5],dolly=[1,1.85,1.8,1.05,1],axial=[-.22,-.48,.5,.2,-.22];
 const mix=(values:number[])=>values[index]+(values[index+1]-values[index])*s;
 const angle=.35+seconds*Math.PI*2/48,distance=fit/mix(dolly);
 const target:[number,number,number]=[bounds.min[0]/1000+size[0]*mix(targets),center[1],center[2]];
 const direction=[mix(axial),Math.sin(angle)*.65,Math.cos(angle)],norm=Math.hypot(...direction);
 const position=target.map((v,i)=>v+direction[i]/norm*distance) as [number,number,number];
 return {position,target,near:Math.max(radius/2000,.00001),far:Math.max(100,fit+radius*20)};
}
