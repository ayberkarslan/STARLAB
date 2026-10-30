import type {Part,Project} from '../../domain/project';
import {NumberInput} from '../NumberInput';
import GeometryFields from '../GeometryFields';

/** Part editing only; numerical algorithms stay in the calculation process. */
export default function StructuralFields({part,project,onChange}:{part:Part;project:Project;onChange:(id:string,patch:Partial<Part>)=>void}){
 const i=part.inputs??{},g=part.geometry??{},parent=project.parts.find(p=>p.id===part.parentId),fin=part.kind==='fins',annular=part.kind==='ring'||part.kind==='bulkhead';
 const update=(patch:Partial<Part>)=>onChange(part.id,patch);
 const supplied=i.massProvided===false?undefined:i.massProvided||part.mass>0?part.mass:undefined;
 return <>
 <div className="engineering-field-grid">
  <NumberInput label={fin?'Kök veter':annular?'Eksenel kalınlık':'Uzunluk'} unit="mm" value={part.length} min={Number.MIN_VALUE} onChange={v=>update({length:v!})}/>
  <NumberInput label={fin?'Kanat dış çapı':'Dış çap'} unit="mm" value={part.diameter} min={Math.max(Number.MIN_VALUE,g.innerDiameter??0,fin?parent?.diameter??0:2*(i.wallThickness??0))} onChange={v=>update({diameter:v!,...(fin&&parent&&g.span!==undefined?{geometry:{...g,span:(v!-parent.diameter)/2}}:{}),...(part.kind==='innerTube'&&g.innerDiameter!==undefined&&i.wallThickness!==undefined?{inputs:{wallThickness:(v!-g.innerDiameter)/2}}:{})})}/>
  {fin&&parent&&<NumberInput label="Açıklık" unit="mm" min={Number.MIN_VALUE} value={(part.diameter-parent.diameter)/2} onChange={v=>update({diameter:parent.diameter+2*v!,geometry:{...g,span:v}})}/>}
  {(part.kind==='innerTube'||part.kind==='ring')&&<NumberInput label="İç çap" unit="mm" optional value={g.innerDiameter} min={part.kind==='innerTube'?Number.MIN_VALUE:0} max={part.diameter} onChange={v=>update({geometry:{...g,innerDiameter:v},...(part.kind==='innerTube'&&v!==undefined&&i.wallThickness!==undefined?{inputs:{wallThickness:(part.diameter-v)/2}}:{})})}/>}
  {part.kind==='mass'?<NumberInput label="Kütle" unit="g" optional value={supplied} min={Number.MIN_VALUE} onChange={v=>update({mass:v??0,inputs:{massProvided:v!==undefined}})}/>:<>
   {!annular&&!(part.kind==='nose'&&g.filled)&&<NumberInput label={fin?'Kanat kalınlığı':'Et kalınlığı'} unit="mm" optional value={i.wallThickness} max={fin?Infinity:part.diameter/2} onChange={v=>update({inputs:{wallThickness:v},...(part.kind==='innerTube'&&v!==undefined?{geometry:{...g,innerDiameter:part.diameter-2*v}}:{})})}/>}
   <NumberInput label="Yoğunluk" unit="kg/m³" optional min={Number.MIN_VALUE} value={i.density} onChange={v=>update({inputs:{density:v}})}/>
  </>}
 </div>
 {(fin||part.kind==='nose')&&<GeometryFields part={part} onChange={update}/>}
 </>;
}
