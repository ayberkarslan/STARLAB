import type {Part,Project} from '../domain/project';
import {tubeInputs} from './tubeInputs';
export const hasStructuralModel=(part:Part)=>['body','innerTube','mass','nose','fins','ring','bulkhead','transition'].includes(part.kind);
const noseProfiles:Record<string,number>={conical:0,tangentOgive:1,elliptical:2,parabolic:3,power:4,haack:5,vonKarman:5};
const sections:Record<string,number>={square:0,rounded:1,airfoil:2,diamond:3,biconvex:4};
/** UI data → SI ports only. Formulae and execution remain in the independent engine. */
export function structuralInputs(part:Part,project?:Project):{model:string;inputs:Record<string,number|undefined>;issue?:string}{
 const g=part.geometry??{},i=part.inputs??{};
 if(part.kind==='body')return {model:'tube',inputs:tubeInputs(part)};
 if(part.kind==='mass')return {model:'mass-component',inputs:{length:part.length/1000,radius:part.diameter/2000,suppliedMass:i.massProvided===false?undefined:i.massProvided||part.mass>0?part.mass/1000:undefined}};
 if(part.kind==='innerTube'||part.kind==='ring'||part.kind==='bulkhead'){
  const diameter=g.innerDiameter,wall=i.wallThickness;
  const issue=part.kind==='innerTube'&&diameter!==undefined&&wall!==undefined&&Math.abs(diameter-(part.diameter-2*wall))>1e-6?'İç çap ve et kalınlığı birbiriyle uyuşmuyor.':undefined;
  return {model:part.kind==='innerTube'?'inner-tube':'annular-cylinder',inputs:{length:part.length/1000,radius:part.diameter/2000,innerRadius:part.kind==='bulkhead'?0:diameter!==undefined?diameter/2000:part.kind==='innerTube'&&wall!==undefined?(part.diameter/2-wall)/1000:undefined,density:i.density},issue};
 }
 if(part.kind==='nose'){
  const profile=g.noseProfile?noseProfiles[g.noseProfile]:undefined,shoulder=(g.shoulderLength??0)>0;
  const issue=profile===undefined?'Hesap için desteklenen bir burun profili seç.':g.filled===undefined?'Burun yapısını seç: dolu veya içi boş.':g.shoulderCapped&&!shoulder?'Omuz kapağı için omuz uzunluğu gerekli.':undefined;
  const parameter=g.noseParameter??(profile===3||profile===4?.5:0);
  return {model:'nose',inputs:{length:part.length/1000,radius:part.diameter/2000,wall:g.filled?0:i.wallThickness===undefined?undefined:i.wallThickness/1000,profile,parameter,filled:g.filled===undefined?undefined:Number(g.filled),shoulderLength:(g.shoulderLength??0)/1000,shoulderRadius:shoulder?(g.shoulderDiameter===undefined?undefined:g.shoulderDiameter/2000):0,shoulderWall:shoulder?(g.shoulderThickness===undefined?undefined:g.shoulderThickness/1000):0,capped:Number(g.shoulderCapped??false),density:i.density},issue};
 }
 if(part.kind==='fins'){
  const parent=project?.parts.find(p=>p.id===part.parentId),span=parent?(part.diameter-parent.diameter)/2:undefined;
  const issue=!parent||parent.kind!=='body'?'Bu modelde kanadı bir gövde tüpüne bağla.':g.finCrossSection==='hex'?'Altıgen kanat kesiti bu üretim modelinde desteklenmiyor.':(g.cantAngle??0)!==0||(g.rotation??0)!==0?'Kant ve dönüş bu yapı modeline henüz bağlı değil.':g.span!==undefined&&span!==undefined&&Math.abs(g.span-span)>1e-6?'Açıklık ve kanat dış çapı birbiriyle uyuşmuyor.':undefined;
  return {model:'trapezoidal-fins',inputs:{root:part.length/1000,tip:g.tipChord===undefined?undefined:g.tipChord/1000,span:span===undefined?undefined:span/1000,sweep:g.sweep===undefined?undefined:g.sweep/1000,thickness:i.wallThickness===undefined?undefined:i.wallThickness/1000,count:g.finCount,sectionCode:g.finCrossSection?sections[g.finCrossSection]:undefined,density:i.density,bodyRadius:parent?parent.diameter/2000:undefined,tabHeight:(g.tabHeight??0)/1000,tabLength:(g.tabLength??0)/1000},issue};
 }
 if(part.kind==='transition'){
  const aft=g.aftDiameter,wall=i.wallThickness;
  const issue=aft===undefined?'Arka çap gerekli.':wall===undefined?'Et kalınlığı gerekli.':wall>=Math.min(part.diameter,aft)/2?'Et kalınlığı iki uç yarıçapından da küçük olmalı.':undefined;
  return {model:'transition',inputs:{length:part.length/1000,foreRadius:part.diameter/2000,aftRadius:aft===undefined?undefined:aft/2000,wall:wall===undefined?undefined:wall/1000,density:i.density},issue};
 }
 throw Error('Bu bileşenin yapısal hesap modeli henüz bağlı değil.');
}
