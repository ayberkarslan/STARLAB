import {useEffect,useState} from 'react';
import {Dialog,Button,SelectMenu,TextField,TabBar,CatalogLayout,CatalogCard} from '@stardrop/ui';
import {Plus} from '@stardrop/ui/icons';
import {componentPresets,catalog,isFin,isStructural,type Kind} from '../domain/componentCatalog';
import {componentTargets} from '../domain/createComponent';
import type {Project} from '../domain/project';
import {focusPath} from '../domain/navigation';
import {noseProfile} from '../domain/geometry';
import './component-library.css';
const categories=['Tümü','Yapı','Aerodinamik','İç yapı','İtki','Kurtarma','Kılavuz'];
export default function ComponentLibrary({open,onClose,project,parentId,onAdd}:{open:boolean;onClose:()=>void;project:Project;parentId:string;onAdd:(parentId:string,presetId:string)=>void}){
 const [category,setCategory]=useState('Tümü'),[query,setQuery]=useState(''),[chosen,setChosen]=useState('body'),[target,setTarget]=useState('');
 const visible=componentPresets.filter(p=>(category==='Tümü'||p.category===category)&&p.label.toLocaleLowerCase('tr').includes(query.toLocaleLowerCase('tr')));
 const active=visible.find(p=>p.id===chosen)??visible[0];const targets=active?componentTargets(project,active.id):[];
 useEffect(()=>{if(open){setCategory('Tümü');setQuery('');setChosen('body')}},[open]);
 useEffect(()=>{const preferred=focusPath(project,parentId).reverse().find(p=>targets.some(t=>t.value===p.id));setTarget(preferred?.id??targets[0]?.value??'')},[active?.id,parentId,project]);
 return <Dialog open={open} onClose={onClose} title="Bileşen ekle" className="component-library">
 <TextField aria-label="Kütüphanede ara" placeholder="Bileşen ara" value={query} onChange={event=>setQuery(event.target.value)}/>
 <TabBar label="Bileşen kategorisi" items={categories.map(c=>({value:c,label:c}))} value={category} onChange={setCategory}/>

 <CatalogLayout preview={<div className="library-preview">{active&&<><ComponentGlyph kind={active.kind} profile={active.geometry?.noseProfile}/><strong>{active.label}</strong><SelectMenu label="Eklenecek yer" value={target} options={targets.length?targets:[{value:'',label:'Önce uygun bir bileşen ekle'}]} disabled={!targets.length} onChange={setTarget}/><Button disabled={!target||!targets.some(t=>t.value===target)} onClick={()=>onAdd(target,active.id)}><Plus size={16}/>Ekle</Button></>}</div>}>
 {visible.map(p=><CatalogCard key={p.id} selected={active?.id===p.id} onClick={()=>setChosen(p.id)} preview={<ComponentGlyph kind={p.kind} profile={p.geometry?.noseProfile}/>}>{p.label}</CatalogCard>)}{!visible.length&&<p>Eşleşme yok.</p>}
 </CatalogLayout>
 </Dialog>;
}
function ComponentGlyph({kind,profile}:{kind:Kind;profile?:Parameters<typeof noseProfile>[2]}){
 const points=noseProfile(60,15,profile);let shape;
 if(kind==='nose')shape=<polygon points={[...points.map(([x,r])=>`${x+10},${25-r}`),...points.slice().reverse().map(([x,r])=>`${x+10},${25+r}`)].join(' ')}/>;
 else if(isFin(kind))shape=<><path d="M10 25h60M18 25L45 6h19v19M18 25L45 44h19V25"/></>;
 else if(kind==='transition')shape=<path d="M10 10L70 18v14L10 40Z"/>;
 else if(['ring','engineBlock','sleeve'].includes(kind))shape=<><ellipse cx="40" cy="25" rx="17" ry="20"/><ellipse cx="40" cy="25" rx="9" ry="12"/></>;
 else if(isStructural(kind))shape=<><rect x="30" y="4" width="20" height="14"/><path d="M40 18v12M15 30h50M15 30v7M65 30v7"/><rect x="7" y="37" width="16" height="10"/><rect x="57" y="37" width="16" height="10"/></>;
 else if(kind==='parachute')shape=<path d="M12 24a28 20 0 0156 0ZM12 24L40 45 68 24M40 24v21"/>;
 else if(kind==='cord')shape=<path d="M9 37c0-25 21-33 21-15s-19 29-5 18S42 4 48 12s-6 36 22 0"/>;
 else if(kind==='tubeFins')shape=<>{[18,40,62].map(x=><circle key={x} cx={x} cy="25" r="10"/>)}</>;
 else shape=<><rect x="12" y="12" width="56" height="26"/>{['innerTube','motor','lug'].includes(kind)&&<path d="M12 18h56M12 32h56"/>}</>;
 return <svg viewBox="0 0 80 50" fill="none" stroke="currentColor" strokeWidth="1.2" aria-hidden="true">{shape}</svg>;
}
