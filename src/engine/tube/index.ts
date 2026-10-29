/// <reference types="vite/client" />
import {CalculationGraph,type CalculationNode} from '../graph';
import {tubeSources} from './contracts';
import type {Part} from '../../domain/project';
// A new file exporting a node is discovered without editing the scheduler or UI.
const modules=import.meta.glob<CalculationNode>('./nodes/*.ts',{eager:true,import:'default'});
export const tubeNodes=Object.values(modules);
export const createTubeGraph=()=>new CalculationGraph(tubeSources,tubeNodes);
/** UI values → SI. No geometry estimates, density fallback or physics here. */
export function tubeInputs(part:Part){return {length:part.length/1000,radius:part.diameter/2000,wall:part.inputs?.wallThickness===undefined?undefined:part.inputs.wallThickness/1000,density:part.inputs?.density}}
