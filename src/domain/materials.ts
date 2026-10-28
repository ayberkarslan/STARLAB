import type {Kind} from './componentCatalog';
export type MaterialType='bulk'|'surface'|'line';
export interface MaterialData {key:string;name:string;type:MaterialType;density:number;shearModulus:number;group:string}
export const densityUnits:Record<MaterialType,string>={bulk:'kg/m³',surface:'kg/m²',line:'kg/m'};
export const densityKeys={bulk:'density',surface:'surfaceDensity',line:'linearDensity'} as const;
export function materialType(kind:Kind):MaterialType{return kind==='parachute'?'surface':kind==='cord'?'line':'bulk'}
export function validateMaterial(value:unknown):asserts value is MaterialData {
 const m=value as MaterialData;
 if(!m||typeof m.key!=='string'||typeof m.name!=='string'||typeof m.group!=='string'||!['bulk','surface','line'].includes(m.type)||!Number.isFinite(m.density)||m.density<=0||!Number.isFinite(m.shearModulus)||m.shearModulus<0)throw Error('Malzeme kaydı geçersiz.');
}
