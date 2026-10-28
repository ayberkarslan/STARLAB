import {isStructural,isShell,isFin} from '../domain/componentCatalog';
import {useState} from 'react';
import {IconButton,SelectMenu,TextField} from '@stardrop/ui';
import {Focus,Trash2,X,Layers,FolderPlus} from 'lucide-react';
import {catalog,type Part} from '../domain/project';
import type {PartInputs} from '../domain/partInputs';
import {NumberInput} from './PartInspector';

interface Props {parts:Part[];onChange:(patch:Partial<Part>)=>void;onSelect:(id:string)=>void;onRemove:(id:string)=>void;onFocus:()=>void;onDelete:()=>void;onGroup?:()=>void}
export default function SelectionInspector({parts,onChange,onSelect,onRemove,onFocus,onDelete,onGroup}:Props){
 const [tab,setTab]=useState('geometry');
 const common=(get:(p:Part)=>unknown)=>{const first=get(parts[0]);return {value:first,mixed:parts.some(p=>!Object.is(get(p),first))}};
 const allPhysical=parts.every(p=>!isStructural(p.kind));
 const allShell=parts.every(p=>isShell(p.kind)),allFins=parts.every(p=>isFin(p.kind));
 const number=(key:'length'|'diameter'|'offset'|'mass'|keyof PartInputs,label:string,unit:string,input=false,optional=false,signed=false,min=0,max=Infinity)=>{
  const result=common(p=>key==='mass'?(p.inputs?.massProvided===false?undefined:p.inputs?.massProvided||p.mass>0?p.mass:undefined):input?p.inputs?.[key as keyof PartInputs]:p[key as 'length']);
  return <NumberInput key={key} label={label} unit={unit} value={result.value as number|undefined} mixed={result.mixed} optional={optional} signed={signed} min={min} max={max} onChange={value=>onChange(key==='mass'?{mass:value??0,inputs:{massProvided:value!==undefined}}:input?{inputs:{[key]:value}}:{[key]:value})}/>;
 };
 const material=common(p=>p.material==='Tanımlanmadı'?'':p.material),scope=common(p=>p.inputs?.massScope??'component');
 return <aside className="part-inspector selection-inspector" aria-label="Ortak bileşen özellikleri">
 <div className="inspector-toolbar"><span><Layers size={14}/> {parts.length} bileşen</span><div>{onGroup&&<IconButton size="sm" variant="ghost" label="Alt sistem oluştur" onClick={onGroup}><FolderPlus size={15}/></IconButton>}<IconButton size="sm" variant="ghost" label="Seçime odaklan" onClick={onFocus}><Focus size={15}/></IconButton><IconButton size="sm" variant="ghost" label="Seçili bileşenleri sil" disabled={parts.some(p=>p.kind==='rocket')} onClick={onDelete}><Trash2 size={15}/></IconButton></div></div>
 <div className="selection-chips" aria-label="Seçili bileşenler">{parts.map(p=><span key={p.id}><button title={`${p.name} · ${catalog[p.kind].label}`} onClick={()=>onSelect(p.id)}>{p.name}</button><button aria-label={`${p.name} seçimden çıkar`} onClick={()=>onRemove(p.id)}><X size={12}/></button></span>)}</div>
 <div className="inspector-tabs" role="group" aria-label="Ortak özellik grubu">{[{id:'geometry',label:'Geometri'},{id:'mass',label:'Kütle'},{id:'aero',label:'Aero'}].map(t=><button key={t.id} aria-pressed={tab===t.id} onClick={()=>setTab(t.id)}>{t.label}</button>)}</div>
 <div className="inspector-fields" key={`${parts.map(p=>p.id).join(':')}:${tab}`}>
 {tab==='geometry'&&!allPhysical&&parts.some(p=>p.kind==='rocket')&&<p className="inspector-reference">Ortak geometri alanı yok.</p>}
 {tab==='geometry'&&<div className="inspector-numbers">{allPhysical&&<>{number('length','Uzunluk','mm',false,false,false,1)}{number('diameter','Dış çap','mm',false,false,false,Math.max(1,...parts.filter(p=>isShell(p.kind)).map(p=>2*(p.inputs?.wallThickness??0))))}</>}{parts.every(p=>p.kind!=='rocket')&&number('offset','Konum','mm',false,false,true)}{(allShell||allFins)&&number('wallThickness',allFins?'Kanat kalınlığı':'Et kalınlığı','mm',true,true,false,0,allShell?Math.min(...parts.map(p=>p.diameter/2)):Infinity)}</div>}
 {tab==='mass'&&<><div className="inspector-numbers">{number('mass','Girilen kütle','g',false,true)}{number('centerOfMass','Kütle merkezi · CG','mm',true,true,true)}{allPhysical&&number('density','Yoğunluk','kg/m³',true,true,false,Number.MIN_VALUE)}</div><label className="inspector-material"><span>Kütle kapsamı</span><SelectMenu label="Kütle kapsamı" value={scope.mixed?'mixed':scope.value as string} options={[...(scope.mixed?[{value:'mixed',label:'Farklı'}]:[]),{value:'component',label:'Yalnız bu bileşen'},{value:'subtree',label:'Alt bileşenlerle birlikte'}]} onChange={value=>{if(value!=='mixed')onChange({inputs:{massScope:value as PartInputs['massScope']}})}}/></label>{allPhysical&&<label className="inspector-material"><span>Malzeme</span><TextField aria-label="Malzeme" value={material.mixed?'':material.value as string} placeholder={material.mixed?'Farklı':'Malzeme adı'} onChange={event=>onChange({material:event.target.value||'Tanımlanmadı'})}/></label>}</>}
 {tab==='aero'&&<div className="inspector-numbers">{number('centerOfPressure','Basınç merkezi · CP','mm',true,true,true)}{number('dragCoefficient','Sürükleme · Cd','—',true,true)}{number('mach','Mach','—',true,true)}{number('angleOfAttack','Hücum açısı','°',true,true,true,-180,180)}{allPhysical&&number('roughness','Pürüzlülük','µm',true,true)}</div>}
 </div></aside>;
}
