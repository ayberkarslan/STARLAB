/// <reference types="vite/client" />
import {CalculationGraph,type CalculationNode} from '../graph';
import {tubeSources} from './contracts';
// A new file exporting a node is discovered without editing the scheduler or UI.
const modules=import.meta.glob<CalculationNode>('./nodes/*.ts',{eager:true,import:'default'});
export const tubeNodes=Object.values(modules);
export const createTubeGraph=()=>new CalculationGraph(tubeSources,tubeNodes);
export {tubeInputs} from '../tubeInputs';
