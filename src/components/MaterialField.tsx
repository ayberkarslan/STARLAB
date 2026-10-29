import {SelectMenu,TextField} from '@stardrop/ui';
import {NumberInput} from './NumberInput';
import type {Part} from '../domain/project';
import {legacyMaterials} from '../domain/materialCatalog';
import {densityKeys,type MaterialType} from '../domain/materials';
export default function MaterialField({material,materialData,type,onChange,mixed=false,shearModulus,modulusMixed=false}:{material:string;materialData?:Part['materialData'];type:MaterialType;onChange:(patch:Partial<Part>)=>void;mixed?:boolean;shearModulus?:number;modulusMixed?:boolean}){
 const value=mixed?'mixed':materialData?.key??(material!=='Tanımlanmadı'?'custom':'');
 return <div className="inspector-material material-field"><span>Malzeme</span><SelectMenu label="Malzeme kataloğu" searchable value={value} options={[...(mixed?[{value:'mixed',label:'Farklı'}]:[]),{value:'',label:'Belirtilmedi'},{value:'custom',label:'Özel malzeme'},...legacyMaterials.filter(m=>m.type===type).map(m=>({value:m.key,label:m.name,searchText:`${m.name} ${m.group}`}))]} onChange={key=>{
  if(key==='mixed')return;
  const m=legacyMaterials.find(m=>m.key===key);
  onChange(m?{material:m.name,materialData:{...m},inputs:{[densityKeys[type]]:m.density,shearModulus:type==='bulk'&&m.shearModulus>0?m.shearModulus:undefined}}:{material:key==='custom'?(material==='Tanımlanmadı'?'':material):'Tanımlanmadı',materialData:undefined});
 }}/>{value==='custom'&&<TextField aria-label="Malzeme" placeholder="Malzeme adı" value={material==='Tanımlanmadı'?'':material} onChange={event=>onChange({material:event.target.value,materialData:undefined})}/>}{type==='bulk'&&<NumberInput label="Kayma modülü · G" unit="GPa" optional min={Number.MIN_VALUE} mixed={modulusMixed} value={shearModulus===undefined?undefined:shearModulus/1e9} onChange={value=>onChange({inputs:{shearModulus:value===undefined?undefined:value*1e9}})}/>}</div>;
}
