import {isStructural,isShell,isFin,finCrossSections,type FinCrossSection} from '../domain/componentCatalog';
import {useState} from 'react';
import {IconButton,SelectMenu,TextField,SelectionChips} from '@stardrop/ui';
import {Focus,Trash2,X,Layers,FolderPlus} from '@stardrop/ui/icons';
import {catalog,type Part} from '../domain/project';
import type {PartInputs} from '../domain/partInputs';
import {NumberInput} from './NumberInput';
import MaterialField from './MaterialField';
import {materialType,densityKeys,densityUnits} from '../domain/materials';

interface Props {parts:Part[];onChange:(patch:Partial<Part>)=>void;onSelect:(id:string)=>void;onRemove:(id:string)=>void;onFocus:()=>void;onDelete:()=>void;onGroup?:()=>void}
export default function SelectionInspector({parts,onChange,onSelect,onRemove,onFocus,onDelete,onGroup}:Props){
 const [tab,setTab]=useState('geometry');
 const common=(get:(p:Part)=>unknown)=>{const first=get(parts[0]);return {value:first,mixed:parts.some(p=>!Object.is(get(p),first))}};
 const allPhysical=parts.every(p=>!isStructural(p.kind));
 const densityType=materialType(parts[0].kind),sameDensityType=parts.every(p=>materialType(p.kind)===densityType);
 const allShell=parts.every(p=>isShell(p.kind)),allFins=parts.every(p=>isFin(p.kind));
 const number=(key:'length'|'diameter'|'offset'|'mass'|keyof PartInputs,label:string,unit:string,input=false,optional=false,signed=false,min=0,max=Infinity)=>{
  const result=common(p=>key==='mass'?(p.inputs?.massProvided===false?undefined:p.inputs?.massProvided||p.mass>0?p.mass:undefined):input?p.inputs?.[key as keyof PartInputs]:p[key as 'length']);
  return <NumberInput key={key} label={label} unit={unit} value={result.value as number|undefined} mixed={result.mixed} optional={optional} signed={signed} min={min} max={max} onChange={value=>onChange(key==='mass'?{mass:value??0,inputs:{massProvided:value!==undefined}}:input?{inputs:{[key]:value}}:{[key]:value})}/>;
 };
 const modulus=common(p=>p.inputs?.shearModulus),section=common(p=>p.geometry?.finCrossSection),filled=common(p=>p.geometry?.filled);
 const geometryNumber=(key:'tabHeight'|'tabLength',label:string)=>{const result=common(p=>p.geometry?.[key]);return <NumberInput key={key} label={label} unit="mm" optional mixed={result.mixed} value={result.value as number|undefined} onChange={value=>onChange({geometry:{[key]:value}})}/>};
 const material=common(p=>p.material==='Tanımlanmadı'?'':p.material),scope=common(p=>p.inputs?.massScope??'component');
 return <aside className="part-inspector selection-inspector" aria-label="Ortak bileşen özellikleri">
 <div className="inspector-toolbar"><span><Layers size={14}/> {parts.length} bileşen</span><div>{onGroup&&<IconButton size="sm" variant="ghost" label="Alt sistem oluştur" onClick={onGroup}><FolderPlus size={15}/></IconButton>}<IconButton size="sm" variant="ghost" label="Seçime odaklan" onClick={onFocus}><Focus size={15}/></IconButton><IconButton size="sm" variant="ghost" label="Seçili bileşenleri sil" disabled={parts.some(p=>p.kind==='rocket')} onClick={onDelete}><Trash2 size={15}/></IconButton></div></div>
 <SelectionChips label="Seçili bileşenler" items={parts.map(p=>({id:p.id,label:p.name}))} onSelect={onSelect} onRemove={onRemove}/>

 <div className="inspector-tabs" role="group" aria-label="Ortak özellik grubu">{[{id:'geometry',label:'Geometri'},{id:'mass',label:'Kütle'},{id:'aero',label:'Aero'}].map(t=><button key={t.id} aria-pressed={tab===t.id} onClick={()=>setTab(t.id)}>{t.label}</button>)}</div>
 <div className="inspector-fields" key={`${parts.map(p=>p.id).join(':')}:${tab}`}>
 {tab==='geometry'&&!allPhysical&&parts.some(p=>p.kind==='rocket')&&<p className="inspector-reference">Ortak geometri alanı yok.</p>}
 {tab==='geometry'&&allFins&&<label className="inspector-material"><span>Kanat kesiti</span><SelectMenu label="Kanat kesiti" value={section.mixed?'mixed':section.value as string??'unknown'} options={[...(section.mixed?[{value:'mixed',label:'Farklı'}]:[]),{value:'unknown',label:'Belirtilmedi'},...finCrossSections]} onChange={value=>{if(value!=='mixed')onChange({geometry:{finCrossSection:value==='unknown'?undefined:value as FinCrossSection}})}}/></label>}
 {tab==='geometry'&&parts.every(p=>p.kind==='nose')&&<label className="inspector-material"><span>Burun yapısı</span><SelectMenu label="Burun yapısı" value={filled.mixed?'mixed':filled.value===undefined?'unknown':filled.value?'solid':'shell'} options={[...(filled.mixed?[{value:'mixed',label:'Farklı'}]:[]),{value:'unknown',label:'Belirtilmedi'},{value:'shell',label:'İçi boş'},{value:'solid',label:'Dolu'}]} onChange={value=>{if(value!=='mixed')onChange({geometry:{filled:value==='unknown'?undefined:value==='solid'}})}}/></label>}
 {tab==='geometry'&&<div className="inspector-numbers">{allPhysical&&<>{number('length','Uzunluk','mm',false,false,false,1)}{number('diameter','Dış çap','mm',false,false,false,Math.max(1,...parts.map(p=>p.geometry?.innerDiameter??0),...parts.filter(p=>isShell(p.kind)).map(p=>2*(p.inputs?.wallThickness??0))))}</>}{parts.every(p=>p.kind!=='rocket')&&number('offset','Konum','mm',false,false,true)}{(allShell||allFins)&&number('wallThickness',allFins?'Kanat kalınlığı':'Et kalınlığı','mm',true,true,false,0,allShell?Math.min(...parts.map(p=>p.diameter/2)):Infinity)}{allFins&&<>{geometryNumber('tabHeight','Kanat dili yüksekliği')}{geometryNumber('tabLength','Kanat dili uzunluğu')}</>}</div>}
 {tab==='mass'&&<><div className="inspector-numbers">{number('mass','Girilen kütle','g',false,true)}{number('centerOfMass','Kütle merkezi · CG','mm',true,true,true)}{allPhysical&&sameDensityType&&number(densityKeys[densityType],'Yoğunluk',densityUnits[densityType],true,true,false,Number.MIN_VALUE)}</div><label className="inspector-material"><span>Kütle kapsamı</span><SelectMenu label="Kütle kapsamı" value={scope.mixed?'mixed':scope.value as string} options={[...(scope.mixed?[{value:'mixed',label:'Farklı'}]:[]),{value:'component',label:'Yalnız bu bileşen'},{value:'subtree',label:'Alt bileşenlerle birlikte'}]} onChange={value=>{if(value!=='mixed')onChange({inputs:{massScope:value as PartInputs['massScope']}})}}/></label>{allPhysical&&sameDensityType&&<MaterialField material={material.value as string} materialData={parts[0].materialData} mixed={material.mixed} type={densityType} shearModulus={modulus.value as number|undefined} modulusMixed={modulus.mixed} onChange={onChange}/>}</>}
 {tab==='aero'&&<div className="inspector-numbers">{number('centerOfPressure','Basınç merkezi · CP','mm',true,true,true)}{number('dragCoefficient','Sürükleme · Cd','—',true,true)}{number('normalForceSlope','Normal kuvvet · CNa','1/rad',true,true,true)}{number('mach','Mach','—',true,true)}{number('angleOfAttack','Hücum açısı','°',true,true,true,-180,180)}{allPhysical&&number('roughness','Pürüzlülük','µm',true,true)}</div>}
 </div></aside>;
}
