import {catalog,componentPresets} from './componentCatalog';
import {addPart,validate,type Project} from './project';
export const NEW_STAGE='@new-stage';
export function componentTargets(project:Project,presetId:string){
 const preset=componentPresets.find(p=>p.id===presetId);if(!preset)return [];
 const targets=project.parts.filter(p=>catalog[preset.kind].parents.includes(p.kind)).map(p=>({value:p.id,label:p.name}));
 if(catalog[preset.kind].parents.includes('stage'))targets.push({value:NEW_STAGE,label:'Yeni kademe'});
 return targets;
}
export function createComponent(project:Project,parentId:string,presetId:string){
 const preset=componentPresets.find(p=>p.id===presetId);if(!preset)throw Error('Bileşen bulunamadı.');
 let next=project,target=parentId;
 if(parentId===NEW_STAGE){
  if(!catalog[preset.kind].parents.includes('stage'))throw Error('Bu bileşen kademeye bağlanamaz.');
  next=addPart(next,next.parts.find(p=>p.kind==='rocket')!.id,'stage');target=next.parts.at(-1)!.id;
  next={...next,parts:next.parts.map(p=>p.id===target?{...p,name:`Kademe ${String(next.parts.filter(p=>p.kind==='stage').length).padStart(2,'0')}`}:p)};
 }
 next=addPart(next,target,preset.kind);const id=next.parts.at(-1)!.id;
 const geometry=structuredClone({...catalog[preset.kind].geometry,...preset.geometry});
 next={...next,parts:next.parts.map(p=>p.id===id?{...p,name:preset.label,geometry}:p)};
 return {project:validate(next),id};
}
