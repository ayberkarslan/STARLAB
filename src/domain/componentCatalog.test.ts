import {describe,it,expect} from 'vitest';
import {emptyProject,validate} from './project';
import {catalog,componentPresets,noseProfiles,isStructural} from './componentCatalog';
import {legacyMaterials} from './materialCatalog';
import {validateMaterial} from './materials';
import {componentTargets,createComponent,NEW_STAGE} from './createComponent';
import {parseStar,serializeStar} from './starFile';
import {geometryFactories} from '../components/geometry3d';
describe('legacy catalog transfer',()=>{
 it('starts empty and keeps every palette entry constructible',()=>{
  expect(emptyProject().parts).toHaveLength(1);expect(validate(emptyProject())).toEqual(emptyProject());expect(noseProfiles).toHaveLength(8);expect(componentPresets).toHaveLength(28);
  let project=emptyProject();project=createComponent(project,NEW_STAGE,'body').project;
  for(const preset of componentPresets){const target=componentTargets(project,preset.id)[0];expect(target, preset.id).toBeDefined();const created=createComponent(project,target.value,preset.id);expect(validate(created.project)).toEqual(created.project);expect(created.project.parts.at(-1)!.kind).toBe(preset.kind);expect(project.parts).toHaveLength(3);}
 });
 it('preserves all 82 material records with separate density dimensions',()=>{
  expect(legacyMaterials).toHaveLength(82);expect(new Set(legacyMaterials.map(m=>m.key)).size).toBe(82);
  expect(['bulk','surface','line'].map(type=>legacyMaterials.filter(m=>m.type===type).length)).toEqual([32,8,42]);
  for(const m of legacyMaterials)expect(()=>validateMaterial(m)).not.toThrow();
  expect(legacyMaterials.find(m=>m.name==='Alüminyum')?.density).toBe(2700);expect(legacyMaterials.find(m=>m.name==='Ripstop Naylon')?.density).toBe(.067);
 });
 it('renders every physical preset without an unsupported 3d factory',()=>{
  for(const p of componentPresets.filter(p=>!isStructural(p.kind))){const def=catalog[p.kind];const g=geometryFactories[p.kind]!(def.length,def.diameter,60,p.geometry);g.computeBoundingBox();expect(g.boundingBox!.isEmpty(),p.id).toBe(false);expect(Number.isFinite(g.boundingBox!.max.x),p.id).toBe(true);g.dispose();}
 });
 it('round trips profiles, geometry and material snapshots in star files',()=>{
  let p=createComponent(emptyProject(),NEW_STAGE,'nose-haack').project;const part=p.parts.at(-1)!;part.materialData={...legacyMaterials[0]};part.material=part.materialData.name;part.inputs={density:part.materialData.density};part.geometry!.noseParameter=.2;
  const restored=parseStar(serializeStar(p));expect(restored.parts).toEqual(p.parts);
  const invalid=structuredClone(p);invalid.parts.at(-1)!.geometry!.finCount=2.5;expect(()=>validate(invalid)).toThrow();
 });
});
