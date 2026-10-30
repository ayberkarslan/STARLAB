export type Kind='rocket'|'stage'|'assembly'|'pod'|'parallelStage'|'nose'|'body'|'transition'|'fins'|'freeformFins'|'tubeFins'|'motor'|'innerTube'|'ring'|'bulkhead'|'engineBlock'|'sleeve'|'mass'|'parachute'|'cord'|'lug'|'railButton';
export type ComponentCategory='Yapı'|'Aerodinamik'|'İç yapı'|'İtki'|'Kurtarma'|'Kılavuz';
export type NoseProfile='conical'|'tangentOgive'|'secantOgive'|'elliptical'|'parabolic'|'power'|'haack'|'vonKarman';
export type FinCrossSection='square'|'rounded'|'airfoil'|'diamond'|'biconvex'|'hex';
export const finCrossSections:{value:FinCrossSection;label:string}[]=[{value:'square',label:'Düz'},{value:'rounded',label:'Yuvarlatılmış'},{value:'airfoil',label:'Kanat profili'},{value:'diamond',label:'Elmas'},{value:'biconvex',label:'Çift dışbükey'},{value:'hex',label:'Altıgen'}];
export interface ComponentGeometry {filled?:boolean;shoulderCapped?:boolean;finCrossSection?:FinCrossSection;tabHeight?:number;tabLength?:number;noseProfile?:NoseProfile;noseParameter?:number;innerDiameter?:number;aftDiameter?:number;shoulderLength?:number;shoulderDiameter?:number;shoulderThickness?:number;finCount?:number;tipChord?:number;span?:number;sweep?:number;cantAngle?:number;rotation?:number;canopyDiameter?:number;cordLength?:number;lineCount?:number;lineLength?:number;instanceCount?:number;instanceSeparation?:number;radialOffset?:number;points?:[number,number][]}
export function scaleAxialGeometry(geometry:ComponentGeometry|undefined,factor:number){
 if(!geometry||factor===1)return geometry;const next={...geometry};for(const key of ['tipChord','sweep','shoulderLength','instanceSeparation','tabLength'] as const)if(next[key]!==undefined)next[key]=Math.round(next[key]!*factor*1000000)/1000000;return next;
}
export interface ComponentDefinition {label:string;parents:Kind[];length:number;diameter:number;category:ComponentCategory;geometry?:ComponentGeometry}
const containers:Kind[]=['stage','assembly','pod','parallelStage'];
const internal:Kind[]=['body','nose','transition','innerTube','motor','assembly'];
const outer:Kind[]=['body','transition'];
export const catalog:Record<Kind,ComponentDefinition>={
 rocket:{label:'Roket',parents:[],length:0,diameter:0,category:'Yapı'},
 stage:{label:'Kademe',parents:['rocket'],length:0,diameter:0,category:'Yapı'},
 assembly:{label:'Alt sistem',parents:['stage','assembly','pod','parallelStage'],length:0,diameter:0,category:'Yapı'},
 pod:{label:'Pod grubu',parents:outer,length:0,diameter:0,category:'Yapı',geometry:{instanceCount:1,radialOffset:0,rotation:0}},
 parallelStage:{label:'Paralel kademe',parents:outer,length:0,diameter:0,category:'Yapı',geometry:{instanceCount:1,radialOffset:0,rotation:0}},
 body:{label:'Gövde tüpü',parents:containers,length:1000,diameter:60,category:'Yapı'},
 nose:{label:'Burun konisi',parents:containers,length:300,diameter:60,category:'Yapı'},
 transition:{label:'Geçiş konisi',parents:containers,length:100,diameter:60,category:'Yapı',geometry:{aftDiameter:48}},
 fins:{label:'Trapez kanat seti',parents:outer,length:250,diameter:300,category:'Aerodinamik',geometry:{finCount:3,tipChord:100,sweep:100,cantAngle:0}},
 freeformFins:{label:'Serbest kanat seti',parents:outer,length:250,diameter:300,category:'Aerodinamik',geometry:{finCount:3,tipChord:100,sweep:100,cantAngle:0,points:[[0,0],[100,120],[200,120],[250,0]]}},
 tubeFins:{label:'Tüp kanat seti',parents:outer,length:76.2,diameter:25.4,category:'Aerodinamik',geometry:{finCount:6,rotation:0}},
 motor:{label:'Motor yuvası',parents:['body','innerTube','assembly'],length:300,diameter:33,category:'İtki',geometry:{innerDiameter:30}},
 innerTube:{label:'İç tüp',parents:internal,length:300,diameter:33,category:'İç yapı',geometry:{innerDiameter:30}},
 ring:{label:'Merkezleme halkası',parents:internal,length:3,diameter:60,category:'İç yapı',geometry:{innerDiameter:30}},
 bulkhead:{label:'Bölme plakası',parents:internal,length:5,diameter:60,category:'İç yapı'},
 engineBlock:{label:'Motor durdurucu',parents:['body','innerTube','motor','assembly'],length:5,diameter:30,category:'İtki',geometry:{innerDiameter:20}},
 sleeve:{label:'Gövde manşonu',parents:outer,length:50,diameter:62,category:'Yapı',geometry:{innerDiameter:60}},
 mass:{label:'Kütle bileşeni',parents:internal,length:200,diameter:50,category:'İç yapı'},
 parachute:{label:'Paraşüt',parents:internal,length:100,diameter:60,category:'Kurtarma',geometry:{canopyDiameter:600,lineCount:0,lineLength:0}},
 cord:{label:'Kurtarma ipi',parents:internal,length:100,diameter:12,category:'Kurtarma',geometry:{cordLength:3000}},
 lug:{label:'Fırlatma kılavuzu',parents:outer,length:50,diameter:16,category:'Kılavuz',geometry:{innerDiameter:12}},
 railButton:{label:'Ray düğmesi',parents:outer,length:15.5,diameter:14.6,category:'Kılavuz',geometry:{innerDiameter:9.5,instanceCount:1,instanceSeparation:0}},
};
export const isStructural=(kind:Kind)=>['rocket','stage','assembly','pod','parallelStage'].includes(kind);
export const isFin=(kind:Kind)=>kind==='fins'||kind==='freeformFins';
export const isShell=(kind:Kind)=>['body','nose','transition','motor','innerTube','sleeve','lug','tubeFins','engineBlock'].includes(kind);
export const isSerial=(kind:Kind)=>['stage','body','nose','transition'].includes(kind);
export const isInternal=(kind:Kind)=>['motor','innerTube','ring','bulkhead','engineBlock','mass','parachute','cord'].includes(kind);
export const noseProfiles:{value:NoseProfile;label:string}[]=[{value:'conical',label:'Konik'},{value:'tangentOgive',label:'Teğet ogiv'},{value:'secantOgive',label:'Sekant ogiv'},{value:'elliptical',label:'Eliptik'},{value:'parabolic',label:'Parabolik'},{value:'power',label:'Üs profili'},{value:'haack',label:'Haack'},{value:'vonKarman',label:'Von Kármán'}];
export const noseParameterDefaults:Partial<Record<NoseProfile,number>>={secantOgive:.5,parabolic:.5,power:.5,haack:0,vonKarman:0};
export const componentPresets=Object.entries(catalog).filter(([kind])=>kind!=='rocket'&&kind!=='nose').map(([kind,def])=>({id:kind,kind:kind as Kind,label:def.label,category:def.category,geometry:def.geometry})).concat(noseProfiles.map(profile=>({id:`nose-${profile.value}`,kind:'nose' as Kind,label:`Burun · ${profile.label}`,category:'Yapı' as ComponentCategory,geometry:{noseProfile:profile.value}})));
export function validateGeometry(value:unknown){
 if(!value||typeof value!=='object'||Array.isArray(value))throw Error('Parça geometrisi geçersiz.');
 const geometry=value as ComponentGeometry;
 for(const [key,n]of Object.entries(geometry)){
  if(n===undefined)continue;
  if(key==='filled'||key==='shoulderCapped'){if(typeof n!=='boolean')throw Error('Doluluk seçimi geçersiz.');continue;}
  if(key==='finCrossSection'){if(!finCrossSections.some(p=>p.value===n))throw Error('Kanat kesiti geçersiz.');continue;}
  if(key==='noseProfile'){if(!noseProfiles.some(p=>p.value===n))throw Error('Burun profili geçersiz.');continue;}
  if(key==='points'){if(!Array.isArray(n)||n.length<3||n.length>500||!n.every(p=>Array.isArray(p)&&p.length===2&&p.every(Number.isFinite)))throw Error('Kanat noktaları geçersiz.');continue;}
  if(typeof n!=='number'||!Number.isFinite(n)||!['sweep','cantAngle','rotation','radialOffset'].includes(key)&&n<0)throw Error('Geometri alanı geçersiz.');
  if(['finCount','instanceCount'].includes(key)&&(!Number.isInteger(n)||n<1||n>64))throw Error('Parça adedi 1–64 arasında olmalı.');
  if(key==='lineCount'&&(!Number.isInteger(n)||n>10000))throw Error('Askı ipi sayısı geçersiz.');
 }
}
