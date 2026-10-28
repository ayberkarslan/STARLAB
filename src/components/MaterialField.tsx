import {SelectMenu,TextField} from '@stardrop/ui';
import type {Part} from '../domain/project';
import {legacyMaterials} from '../domain/materialCatalog';
import {densityKeys,type MaterialType} from '../domain/materials';
export default function MaterialField({material,materialData,type,onChange,mixed=false}:{material:string;materialData?:Part['materialData'];type:MaterialType;onChange:(patch:Partial<Part>)=>void;mixed?:boolean}){
 const value=mixed?'mixed':materialData?.key??(material!=='Tanımlanmadı'?'custom':'');
 return <div className="inspector-material material-field"><span>Malzeme</span><SelectMenu label="Malzeme kataloğu" searchable value={value} options={[...(mixed?[{value:'mixed',label:'Farklı'}]:[]),{value:'',label:'Belirtilmedi'},{value:'custom',label:'Özel malzeme'},...legacyMaterials.filter(m=>m.type===type).map(m=>({value:m.key,label:m.name,searchText:`${m.name} ${m.group}`}))]} onChange={key=>{
  if(key==='mixed')return;
  const m=legacyMaterials.find(m=>m.key===key);
  onChange(m?{material:m.name,materialData:{...m},inputs:{[densityKeys[type]]:m.density}}:{material:key==='custom'?(material==='Tanımlanmadı'?'':material):'Tanımlanmadı',materialData:undefined});
 }}/>{value==='custom'&&<TextField aria-label="Malzeme" placeholder="Malzeme adı" value={material==='Tanımlanmadı'?'':material} onChange={event=>onChange({material:event.target.value,materialData:undefined})}/>}</div>;
}
