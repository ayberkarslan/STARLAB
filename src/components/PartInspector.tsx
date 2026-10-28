import {useEffect,useState} from 'react';
import {IconButton,SelectMenu,TextField} from '@stardrop/ui';
import {Box,Focus,Plus,Trash2} from 'lucide-react';
import {catalog,type Part} from '../domain/project';

interface Props {part?:Part;onChange:(patch:Partial<Part>)=>void;onAdd:()=>void;onDelete:()=>void;onFocus:()=>void}
export default function PartInspector({part,onChange,onAdd,onDelete,onFocus}:Props){
 if(!part)return <aside className="part-inspector inspector-empty" aria-label="Bileşen özellikleri"><Box size={24}/><span>Bir bileşen seç</span></aside>;
 const physical=!['rocket','stage','assembly'].includes(part.kind);
 const acceptsChildren=Object.values(catalog).some(entry=>entry.parents.includes(part.kind));
 return <aside className="part-inspector" aria-label="Bileşen özellikleri" key={part.id}>
 <div className="inspector-toolbar"><span>{catalog[part.kind].label}</span><div><IconButton size="sm" variant="ghost" label="Odaklan" onClick={onFocus}><Focus size={15}/></IconButton>{acceptsChildren&&<IconButton size="sm" variant="ghost" label="Alt bileşen ekle" onClick={onAdd}><Plus size={15}/></IconButton>}{part.kind!=='rocket'&&<IconButton size="sm" variant="ghost" label="Bileşeni sil" onClick={onDelete}><Trash2 size={15}/></IconButton>}</div></div>
 <div className="inspector-fields" key={part.id}><TextField aria-label="Bileşen adı" value={part.name} onChange={event=>onChange({name:event.target.value})}/>
 <div className="inspector-numbers">{physical&&<><NumberInput label="Uzunluk" unit="mm" value={part.length} onChange={length=>onChange({length})}/><NumberInput label="Çap" unit="mm" value={part.diameter} onChange={diameter=>onChange({diameter})}/></>}{part.kind!=='rocket'&&<NumberInput label="Konum" unit="mm" value={part.offset} signed onChange={offset=>onChange({offset})}/>} {physical&&<NumberInput label="Kütle" unit="g" value={part.mass} onChange={mass=>onChange({mass})}/>}</div>
 {physical&&<label className="inspector-material"><span>Malzeme</span><SelectMenu label="Malzeme" value={part.material} options={Array.from(new Set([part.material,'Tanımlanmadı','Alüminyum','Karbon fiber','Cam elyaf','Kontrplak','Naylon'])).map(value=>({value,label:value}))} onChange={material=>onChange({material})}/></label>}</div>
 </aside>;
}
function NumberInput({label,unit,value,signed=false,onChange}:{label:string;unit:string;value:number;signed?:boolean;onChange:(value:number)=>void}){
 const [draft,setDraft]=useState(String(value));useEffect(()=>setDraft(String(value)),[value]);
 const valid=draft.trim()!==''&&Number.isFinite(Number(draft))&&(signed||Number(draft)>=0);
 return <label className="inspector-number"><span>{label}</span><div><input aria-label={label} inputMode="decimal" value={draft} aria-invalid={!valid} onChange={event=>{const text=event.target.value;setDraft(text);const n=Number(text);if(text.trim()!==''&&Number.isFinite(n)&&(signed||n>=0))onChange(n)}} onBlur={()=>{if(!valid)setDraft(String(value))}}/><small>{unit}</small></div></label>;
}
