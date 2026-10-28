import type {Motor} from '../../domain/motors';
import {fmt} from './shared';
export default function MotorPreview({motor,mini=false}:{motor:Motor;mini?:boolean}){
 const length=480;const diameter=Math.max(40,Math.min(140,length*motor.diameter/motor.length));const y=110-diameter/2;
 return <svg className={mini?'motor-mini':'motor-preview'} viewBox="0 0 660 230" role="img" aria-label={`${motor.name} motor önizlemesi`}>
 <defs><linearGradient id={mini?`mini-${motor.id}`:`case-${motor.id}`} x1="0" y1="0" x2="0" y2="1"><stop offset="0" stopColor="#050505"/><stop offset=".32" stopColor="#3b3b3b"/><stop offset=".5" stopColor="#171717"/><stop offset="1" stopColor="#000"/></linearGradient></defs>
 <line x1="25" x2="630" y1="110" y2="110" stroke="#ffffff30" strokeDasharray="6 7"/>
 <rect x="65" y={y} width={length} height={diameter} rx="6" fill={`url(#${mini?`mini-${motor.id}`:`case-${motor.id}`})`} stroke="white" strokeWidth="1.5"/>
 <path d={`M545 ${y+diameter*.23} L575 ${y+diameter*.1} L575 ${y+diameter*.9} L545 ${y+diameter*.77}`} fill="black" stroke="white"/>
 <path d={`M80 ${y}v${diameter} M525 ${y}v${diameter}`} stroke="white"/>
 {!mini&&<><text x="110" y="115" fill="white" fontSize="16" letterSpacing="3">{motor.name}</text><path d="M65 175v12 H545v-12" fill="none" stroke="#ffffff80"/><text x="305" y="214" textAnchor="middle" fill="white" fontSize="14">{fmt(motor.length,0)} mm · Ø {fmt(motor.diameter,0)} mm</text></>}
 </svg>;
}
