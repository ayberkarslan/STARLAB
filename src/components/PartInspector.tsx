import {isStructural,isShell,isFin} from '../domain/componentCatalog';
import {useEffect,useState} from 'react';
import {IconButton,SelectMenu,TextField,TextArea} from '@stardrop/ui';
import {Box,Focus,Plus,Trash2} from 'lucide-react';
import {catalog,type Part} from '../domain/project';
import type {PartInputs} from '../domain/partInputs';
import {NumberInput} from './NumberInput';
import GeometryFields from './GeometryFields';
import MaterialField from './MaterialField';
import {materialType,densityKeys,densityUnits} from '../domain/materials';

interface Props {part?:Part;onChange:(patch:Partial<Part>)=>void;onAdd:()=>void;onDelete:()=>void;onFocus:()=>void}
export default function PartInspector({part,onChange,onAdd,onDelete,onFocus}:Props){
 const [section,setSection]=useState('geometry');
 if(!part)return <aside className="part-inspector inspector-empty" aria-label="Bileşen özellikleri"><Box size={24}/><span>Bir bileşen seç</span></aside>;
 const physical=!isStructural(part.kind);
 const shell=isShell(part.kind);
 const inputs=part.inputs??{};
 const input=(patch:Partial<PartInputs>)=>onChange({inputs:{...inputs,...patch}});
 const numeric=(key:keyof PartInputs,label:string,unit:string,signed=false,max?:number,min=0)=><NumberInput key={key} label={label} unit={unit} value={inputs[key] as number|undefined} optional signed={signed} max={max} min={min} onChange={value=>input({[key]:value})}/>;
 const acceptsChildren=Object.values(catalog).some(entry=>entry.parents.includes(part.kind));
 return <aside className="part-inspector" aria-label="Bileşen özellikleri" key={part.id}>
 <div className="inspector-toolbar"><span>{catalog[part.kind].label}</span><div><IconButton size="sm" variant="ghost" label="Odaklan" onClick={onFocus}><Focus size={15}/></IconButton>{acceptsChildren&&<IconButton size="sm" variant="ghost" label="Alt bileşen ekle" onClick={onAdd}><Plus size={15}/></IconButton>}{part.kind!=='rocket'&&<IconButton size="sm" variant="ghost" label="Bileşeni sil" onClick={onDelete}><Trash2 size={15}/></IconButton>}</div></div>
 <div className="inspector-tabs" role="group" aria-label="Bileşen veri grubu">{[{id:'geometry',label:'Geometri'},{id:'mass',label:'Kütle'},{id:'aero',label:'Aero'},{id:'notes',label:'Notlar'}].map(s=><button key={s.id} aria-pressed={section===s.id} onClick={()=>setSection(s.id)}>{s.label}</button>)}</div>
 <div className="inspector-fields" key={`${part.id}-${section}`}>
 {section==='geometry'&&<><TextField aria-label="Bileşen adı" value={part.name} onChange={event=>onChange({name:event.target.value})}/><div className="inspector-numbers">{physical&&<><NumberInput label="Uzunluk" unit="mm" value={part.length} onChange={length=>onChange({length:length!})}/><NumberInput label="Dış çap" unit="mm" value={part.diameter} min={Math.max(part.geometry?.innerDiameter??0,shell?(inputs.wallThickness??0)*2:0)} onChange={diameter=>onChange({diameter:diameter!})}/></>}{part.kind!=='rocket'&&<NumberInput label="Konum" unit="mm" value={part.offset} signed onChange={offset=>onChange({offset:offset!})}/>}{(shell||isFin(part.kind))&&numeric('wallThickness',isFin(part.kind)?'Kanat kalınlığı':'Et kalınlığı','mm',false,shell?part.diameter/2:undefined)}</div><GeometryFields part={part} onChange={onChange}/></>}
 {section==='mass'&&<><div className="inspector-numbers"><NumberInput label="Girilen kütle" unit="g" optional value={inputs.massProvided===false?undefined:inputs.massProvided||part.mass>0?part.mass:undefined} onChange={value=>onChange({mass:value??0,inputs:{...inputs,massProvided:value!==undefined}})}/>{numeric('centerOfMass','Kütle merkezi · CG','mm',true)}{physical&&numeric(densityKeys[materialType(part.kind)],'Yoğunluk',densityUnits[materialType(part.kind)],false,undefined,Number.MIN_VALUE)}</div><label className="inspector-material"><span>Kütle kapsamı</span><SelectMenu label="Kütle kapsamı" value={inputs.massScope??'component'} options={[{value:'component',label:'Yalnız bu bileşen'},{value:'subtree',label:'Alt bileşenlerle birlikte'}]} onChange={value=>input({massScope:value as PartInputs['massScope']})}/></label>{physical&&<MaterialField material={part.material} materialData={part.materialData} type={materialType(part.kind)} shearModulus={inputs.shearModulus} onChange={onChange}/>}<p className="inspector-reference">CG: bileşen başlangıcından, +X yönünde.</p></>}
 {section==='aero'&&<><div className="inspector-numbers">{numeric('centerOfPressure','Basınç merkezi · CP','mm',true)}{numeric('dragCoefficient','Sürükleme · Cd','—')}{numeric('normalForceSlope','Normal kuvvet · CNa','1/rad',true)}{numeric('mach','Mach','—')}{numeric('angleOfAttack','Hücum açısı','°',true,180,-180)}{physical&&numeric('roughness','Pürüzlülük','µm')}</div><p className="inspector-reference">CP: bileşen başlangıcından, +X yönünde. Değerler belirtilen akış koşuluna aittir.</p></>}
 {section==='notes'&&<TextArea label="Kaynak / not" aria-label="Bileşen notu" maxLength={10000} value={inputs.notes??''} onChange={event=>input({notes:event.target.value})}/>}
 </div>
 </aside>;
}
