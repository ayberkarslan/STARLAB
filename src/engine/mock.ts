import { CalculationGraph } from './graph';
import type { Project } from '../domain/project';
export const createMockEngine=()=>new CalculationGraph([
 {id:'inventory',dependencies:['structure'],run:v=>(v.structure as Project).parts.filter(p=>!['rocket','stage','assembly'].includes(p.kind)).length},
 {id:'summary',dependencies:['inventory'],run:v=>({partCount:v.inventory})}
]);
