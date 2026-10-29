import type {Bounds} from '../domain/geometry';
export const CINEMATIC_SHOT_SECONDS=10;
// Each shot moves slowly within its own framing. Shot boundaries are hard cuts.
const shots=[
 {target:[.48,.52],angle:[.25,1.15],dolly:[1,1.06],axial:[-.22,-.12]},
 {target:[.13,.2],angle:[2.65,3.55],dolly:[1.85,1.98],axial:[-.48,-.38]},
 {target:[.84,.78],angle:[-.85,.05],dolly:[1.8,1.68],axial:[.5,.4]},
 {target:[.5,.47],angle:[4,4.9],dolly:[1.04,1],axial:[.2,.12]},
] as const;
/** Hero, nose detail, tail detail, reverse hero. No interpolation across cuts. */
export function cinematicPose(bounds:Bounds,aspect:number,seconds:number){
 const size=bounds.max.map((v,i)=>(v-bounds.min[i])/1000),center=bounds.max.map((v,i)=>(v+bounds.min[i])/2000);
 const radius=Math.max(Math.hypot(...size)/2,.03),halfFov=16*Math.PI/180;
 const fit=Math.max(size[0]/2/(Math.tan(halfFov)*Math.max(aspect,.2)),Math.hypot(size[1],size[2])/2/Math.tan(halfFov),.1)*1.2+radius*.1;
 const time=Math.max(0,seconds)/CINEMATIC_SHOT_SECONDS,index=Math.floor(time)%shots.length,t=time-Math.floor(time),shot=shots[index];
 const mix=(values:readonly [number,number])=>values[0]+(values[1]-values[0])*t;
 const angle=mix(shot.angle),distance=fit/mix(shot.dolly);
 const target:[number,number,number]=[bounds.min[0]/1000+size[0]*mix(shot.target),center[1],center[2]];
 const direction=[mix(shot.axial),Math.sin(angle)*.65,Math.cos(angle)],norm=Math.hypot(...direction);
 const position=target.map((v,i)=>v+direction[i]/norm*distance) as [number,number,number];
 return {position,target,near:Math.max(radius/2000,.00001),far:Math.max(100,fit+radius*20)};
}
