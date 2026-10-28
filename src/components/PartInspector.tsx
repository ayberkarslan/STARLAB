import {useEffect,useState} from 'react';
import {IconButton,SelectMenu,TextField,TextArea} from '@stardrop/ui';
import {Box,Focus,Plus,Trash2} from 'lucide-react';
import {catalog,type Part} from '../domain/project';
import type {PartInputs} from '../domain/partInputs';

interface Props {part?:Part;onChange:(patch:Partial<Part>)=>void;onAdd:()=>void;onDelete:()=>void;onFocus:()=>void}
export default function PartInspector({part,onChange,onAdd,onDelete,onFocus}:Props){
 const [section,setSection]=useState('geometry');
 if(!part)return <aside className="part-inspector inspector-empty" aria-label="Bileşen özellikleri"><Box size={24}/><span>Bir bileşen seç</span></aside>;
 const physical=!['rocket','stage','assembly'].includes(part.kind);
 const shell=['body','nose','motor'].includes(part.kind);
 const inputs=part.inputs??{};
 const input=(patch:Partial<PartInputs>)=>onChange({inputs:{...inputs,...patch}});
 const numeric=(key:keyof PartInputs,label:string,unit:string,signed=false,max?:number,min=0)=><NumberInput key={key} label={label} unit={unit} value={inputs[key] as number|undefined} optional signed={signed} max={max} min={min} onChange={value=>input({[key]:value})}/>;
 const acceptsChildren=Object.values(catalog).some(entry=>entry.parents.includes(part.kind));
 return <aside className="part-inspector" aria-label="Bileşen özellikleri" key={part.id}>
 <div className="inspector-toolbar"><span>{catalog[part.kind].label}</span><div><IconButton size="sm" variant="ghost" label="Odaklan" onClick={onFocus}><Focus size={15}/></IconButton>{acceptsChildren&&<IconButton size="sm" variant="ghost" label="Alt bileşen ekle" onClick={onAdd}><Plus size={15}/></IconButton>}{part.kind!=='rocket'&&<IconButton size="sm" variant="ghost" label="Bileşeni sil" onClick={onDelete}><Trash2 size={15}/></IconButton>}</div></div>
 <div className="inspector-tabs" role="group" aria-label="Bileşen veri grubu">{[{id:'geometry',label:'Geometri'},{id:'mass',label:'Kütle'},{id:'aero',label:'Aero'},{id:'notes',label:'Notlar'}].map(s=><button key={s.id} aria-pressed={section===s.id} onClick={()=>setSection(s.id)}>{s.label}</button>)}</div>
 <div className="inspector-fields" key={`${part.id}-${section}`}>
 {section==='geometry'&&<><TextField aria-label="Bileşen adı" value={part.name} onChange={event=>onChange({name:event.target.value})}/><div className="inspector-numbers">{physical&&<><NumberInput label="Uzunluk" unit="mm" value={part.length} onChange={length=>onChange({length:length!})}/><NumberInput label="Dış çap" unit="mm" value={part.diameter} min={shell?(inputs.wallThickness??0)*2:0} onChange={diameter=>onChange({diameter:diameter!})}/></>}{part.kind!=='rocket'&&<NumberInput label="Konum" unit="mm" value={part.offset} signed onChange={offset=>onChange({offset:offset!})}/>}{(shell||part.kind==='fins')&&numeric('wallThickness',part.kind==='fins'?'Kanat kalınlığı':'Et kalınlığı','mm',false,shell?part.diameter/2:undefined)}</div></>}
 {section==='mass'&&<><div className="inspector-numbers"><NumberInput label="Girilen kütle" unit="g" optional value={inputs.massProvided===false?undefined:inputs.massProvided||part.mass>0?part.mass:undefined} onChange={value=>onChange({mass:value??0,inputs:{...inputs,massProvided:value!==undefined}})}/>{numeric('centerOfMass','Kütle merkezi · CG','mm',true)}{physical&&numeric('density','Yoğunluk','kg/m³',false,undefined,Number.MIN_VALUE)}</div><label className="inspector-material"><span>Kütle kapsamı</span><SelectMenu label="Kütle kapsamı" value={inputs.massScope??'component'} options={[{value:'component',label:'Yalnız bu bileşen'},{value:'subtree',label:'Alt bileşenlerle birlikte'}]} onChange={value=>input({massScope:value as PartInputs['massScope']})}/></label>{physical&&<label className="inspector-material"><span>Malzeme</span><TextField aria-label="Malzeme" value={part.material==='Tanımlanmadı'?'':part.material} placeholder="Malzeme adı" onChange={event=>onChange({material:event.target.value||'Tanımlanmadı'})}/></label>}<p className="inspector-reference">CG: bileşen başlangıcından, +X yönünde.</p></>}
 {section==='aero'&&<><div className="inspector-numbers">{numeric('centerOfPressure','Basınç merkezi · CP','mm',true)}{numeric('dragCoefficient','Sürükleme · Cd','—')}{numeric('mach','Mach','—')}{numeric('angleOfAttack','Hücum açısı','°',true,180,-180)}{physical&&numeric('roughness','Pürüzlülük','µm')}</div><p className="inspector-reference">CP: bileşen başlangıcından, +X yönünde. Değerler belirtilen akış koşuluna aittir.</p></>}
 {section==='notes'&&<TextArea label="Kaynak / not" aria-label="Bileşen notu" maxLength={10000} value={inputs.notes??''} onChange={event=>input({notes:event.target.value})}/>}
 </div>
 </aside>;
}
export function NumberInput({label,unit,value,optional=false,signed=false,min=0,max=Infinity,mixed=false,onChange}:{label:string;unit:string;value?:number;optional?:boolean;signed?:boolean;min?:number;max?:number;mixed?:boolean;onChange:(value:number|undefined)=>void}){
 const [draft,setDraft]=useState(mixed||value===undefined?'':String(value));useEffect(()=>setDraft(mixed||value===undefined?'':String(value)),[value,mixed]);
 const parse=(text:string)=>Number(text.replace(',','.'));
 const accepts=(text:string)=>optional&&text.trim()===''||text.trim()!==''&&Number.isFinite(parse(text))&&parse(text)<=max&&(signed&&min===0||parse(text)>=min);
 const valid=mixed&&draft===''||accepts(draft);
 return <label className="inspector-number"><span>{label}</span><div><input aria-label={label} inputMode="decimal" placeholder={mixed?'Farklı':optional?'—':undefined} value={draft} aria-invalid={!valid} title={mixed?'Seçili bileşenlerde farklı değerler var':optional?'Boş: belirtilmedi':''} onChange={event=>{const text=event.target.value;setDraft(text);if(accepts(text))onChange(text.trim()===''?undefined:parse(text))}} onBlur={()=>{if(!valid)setDraft(mixed||value===undefined?'':String(value))}}/><small>{unit}</small></div></label>;
}
