import type {Project} from '../domain/project';
import {isStructural,isShell,isFin} from '../domain/componentCatalog';
import {materialType,densityKeys} from '../domain/materials';
import {configurationIssues,type FlightWorkspace} from '../domain/flight';

export interface InputGap {target:string;fields:string[];partId?:string}
export interface InputGroup {id:'geometry'|'aero'|'motors'|'environment'|'recovery';label:string;gaps:InputGap[]}
/** Presence checks for the basic input collection flow, never solver admission
 * or a claim of numerical validity. Derived CG/CP/Cd/CNa are not required inputs.
 */
export function inputReadiness(project:Project,flight:FlightWorkspace):InputGroup[]{
 const physical=project.parts.filter(p=>!isStructural(p.kind));
 const geometry:InputGap[]=physical.flatMap(p=>{
  const fields:string[]=[],i=p.inputs??{},g=p.geometry??{};
  if(p.length<=0)fields.push('Uzunluk');if(p.diameter<=0)fields.push('Çap');
  if(p.kind==='nose'&&g.filled===undefined)fields.push('Burun yapısı');
  if(isShell(p.kind)&&g.filled!==true&&g.innerDiameter===undefined&&i.wallThickness===undefined)fields.push('Et kalınlığı');
  if(isFin(p.kind)){if(i.wallThickness===undefined)fields.push('Kanat kalınlığı');if(g.finCrossSection===undefined)fields.push('Kanat kesiti');if(g.finCount===undefined)fields.push('Kanat sayısı');}
  const supplied=i.massProvided===true||i.massProvided!==false&&p.mass>0;
  if(!supplied&&(p.kind==='mass'||i[densityKeys[materialType(p.kind)]]===undefined))fields.push(p.kind==='mass'?'Kütle':'Kütle veya yoğunluk');
  return fields.length?[{target:p.name,partId:p.id,fields}]:[];
 });
 if(!physical.length)geometry.push({target:'Tasarım',fields:['Bileşen ekle']});
 const aeroFields:string[]=[],a=project.aerodynamics??{};
 if(a.referenceDiameter===undefined)aeroFields.push('Referans çapı');
 if(a.mach===undefined)aeroFields.push('Mach');if(a.angleOfAttack===undefined)aeroFields.push('Hücum açısı');
 const motors:InputGap[]=flight.configurations.flatMap(c=>configurationIssues(project,flight,c).map(label=>({target:c.name,fields:[label]})));
 const environment:InputGap[]=flight.cases.flatMap(c=>{
  const e=c.environment,fields:string[]=[];
  if(e.atmosphereModel===undefined)fields.push('Atmosfer modeli');
  if(e.atmosphereModel==='profile'&&!e.atmosphereProfile?.length)fields.push('Atmosfer profili');
  if(e.windModel==='profile'&&!e.windProfile?.length)fields.push('Rüzgâr profili');
  if(['powerLaw','logarithmic'].includes(e.windModel??'')){
   if(e.windReferenceHeight===undefined)fields.push('Referans yüksekliği');
   if(e.windMinimumHeight===undefined)fields.push('Asgari yükseklik');
   if(e.windModel==='powerLaw'&&e.windShearExponent===undefined)fields.push('Rüzgâr üssü');
   if(e.windModel==='logarithmic'&&e.windRoughnessLength===undefined)fields.push('Yüzey pürüzlülüğü');
  }
  if(e.turbulence?.enabled)for(const [key,label]of [['rms','RMS hız'],['alpha','Spektrum katsayısı'],['poles','Kutup sayısı'],['sampleRate','Örnekleme'],['seed','Rastgelelik tohumu']] as const)if(e.turbulence[key]===undefined)fields.push(label);
  return fields.length?[{target:c.name,fields}]:[];
 });
 if(!flight.cases.length)environment.push({target:'Uçuş',fields:['Simülasyon ekle']});
 const recovery:InputGap[]=flight.configurations.flatMap(c=>project.parts.filter(p=>p.kind==='parachute').flatMap(p=>{
  const r=c.recovery[p.id];if(r?.event==='never')return [];
  const fields:string[]=[];if(r?.dragCoefficient===undefined)fields.push('Açık paraşüt Cd');if(r?.inflationTime===undefined)fields.push('Şişme süresi');
  if(r?.referenceArea===undefined&&!p.geometry?.canopyDiameter)fields.push('Açık paraşüt çapı veya alanı');
  return fields.length?[{target:`${c.name} · ${p.name}`,partId:p.id,fields}]:[];
 }));
 return [{id:'geometry',label:'Geometri ve kütle',gaps:geometry},{id:'aero',label:'Aero incelemesi',gaps:aeroFields.length?[{target:'Ortak koşullar',fields:aeroFields}]:[]},{id:'motors',label:'Motor atamaları',gaps:motors},{id:'environment',label:'Çevre',gaps:environment},{id:'recovery',label:'Kurtarma',gaps:recovery}];
}
