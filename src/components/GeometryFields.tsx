import {SelectMenu,TextArea} from '@stardrop/ui';
import {useState,useEffect} from 'react';
import {isFin,noseProfiles,type ComponentGeometry} from '../domain/componentCatalog';
import type {Part} from '../domain/project';
import {NumberInput} from './NumberInput';
export default function GeometryFields({part,onChange}:{part:Part;onChange:(patch:Partial<Part>)=>void}){
 const geometry=part.geometry??{};const set=(patch:Partial<ComponentGeometry>)=>onChange({geometry:{...geometry,...patch}});
 const field=(key:keyof ComponentGeometry,label:string,unit='mm',signed=false,min=0,max=Infinity)=><NumberInput key={key} label={label} unit={unit} value={geometry[key] as number|undefined} optional signed={signed} integer={['finCount','instanceCount','lineCount'].includes(key)} min={min} max={max} onChange={value=>set({[key]:value})}/>;
 return <>
 {part.kind==='nose'&&<label className="inspector-material"><span>Burun profili</span><SelectMenu label="Burun profili" value={geometry.noseProfile??'legacy'} options={[{value:'legacy',label:'Mevcut profil'},...noseProfiles]} onChange={value=>set({noseProfile:value==='legacy'?undefined:value as ComponentGeometry['noseProfile']})}/></label>}
 <div className="inspector-numbers">
 {part.kind==='nose'&&<>{field('noseParameter','Profil parametresi','—',false,0,1)}{field('shoulderLength','Omuz uzunluğu')}{field('shoulderDiameter','Omuz çapı')}{field('shoulderThickness','Omuz et kalınlığı')}</>}
 {part.kind==='transition'&&field('aftDiameter','Arka çap')}
 {['innerTube','motor','ring','engineBlock','sleeve','lug','railButton'].includes(part.kind)&&field('innerDiameter','İç çap','mm',false,0,part.diameter)}
 {(isFin(part.kind)||part.kind==='tubeFins')&&<>{field('finCount','Kanat sayısı','adet',false,1,64)}{field('rotation','Dönüş','°',true)}</>}
 {isFin(part.kind)&&<>{field('tipChord','Uç veter')}{field('sweep','Süpürme','mm',true)}{field('cantAngle','Kant açısı','°',true)}</>}
 {part.kind==='parachute'&&<>{field('canopyDiameter','Açık paraşüt çapı')}{field('lineCount','Askı ipi sayısı','adet')}{field('lineLength','Askı ipi uzunluğu')}</>}
 {part.kind==='cord'&&field('cordLength','Açık ip uzunluğu')}
 {['pod','parallelStage','railButton'].includes(part.kind)&&<>{field('instanceCount','Tekrar sayısı','adet',false,1,64)}{field('instanceSeparation','Tekrar aralığı')}</>}
 {['pod','parallelStage'].includes(part.kind)&&<>{field('radialOffset','Radyal konum','mm',true)}{field('rotation','Açısal konum','°',true)}</>}
 </div>{part.kind==='freeformFins'&&<Points value={geometry.points} onChange={points=>set({points})}/>}
 </>;
}
function Points({value,onChange}:{value?:[number,number][];onChange:(value:[number,number][])=>void}){
 const [text,setText]=useState(JSON.stringify(value??[]));useEffect(()=>setText(JSON.stringify(value??[])),[value]);
 let valid=false;try{const points=JSON.parse(text);valid=Array.isArray(points)&&points.length>=3&&points.length<=500&&points.every(p=>Array.isArray(p)&&p.length===2&&p.every(Number.isFinite))}catch{}
 return <TextArea label="Kanat noktaları · mm" aria-label="Kanat noktaları" value={text} error={!valid?'En az üç [x,y] noktası gerekli.':undefined} onChange={event=>{setText(event.target.value)}} onBlur={()=>{if(valid)onChange(JSON.parse(text))}}/>;
}
