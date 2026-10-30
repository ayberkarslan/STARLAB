import type {Part} from '../domain/project';
import {tubeInputs} from './tubeInputs';

export const hasStructuralModel=(part:Part)=>['body','innerTube','mass'].includes(part.kind);
/** UI data → SI ports only. Physical kernels and model discovery remain native. */
export function structuralInputs(part:Part):{model:string;inputs:Record<string,number|undefined>;issue?:string}{
 if(part.kind==='body')return {model:'tube',inputs:tubeInputs(part)};
 if(part.kind==='mass')return {model:'mass-component',inputs:{length:part.length/1000,radius:part.diameter/2000,suppliedMass:part.inputs?.massProvided===false?undefined:part.inputs?.massProvided||part.mass>0?part.mass/1000:undefined}};
 if(part.kind==='innerTube'){
  const diameter=part.geometry?.innerDiameter,wall=part.inputs?.wallThickness;
  const issue=diameter!==undefined&&wall!==undefined&&Math.abs(diameter-(part.diameter-2*wall))>1e-6?'İç çap ve et kalınlığı birbiriyle uyuşmuyor.':undefined;
  return {model:'inner-tube',inputs:{length:part.length/1000,radius:part.diameter/2000,innerRadius:diameter!==undefined?diameter/2000:wall!==undefined?(part.diameter/2-wall)/1000:undefined,density:part.inputs?.density},issue};
 }
 throw Error('Bu bileşenin yapısal hesap modeli henüz bağlı değil.');
}
