import type {Part} from '../domain/project';
/** Design inputs → SI request. No calculation kernels or node execution here. */
export function tubeInputs(part:Part){return {length:part.length/1000,radius:part.diameter/2000,wall:part.inputs?.wallThickness===undefined?undefined:part.inputs.wallThickness/1000,density:part.inputs?.density}}
