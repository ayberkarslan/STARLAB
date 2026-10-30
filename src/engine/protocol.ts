import type {GraphSnapshot} from './graph';
export interface PortDescription {id:string;unit:string;frame?:string|null;kind?:'number-array'}
export interface SourceDescription {id:string;label:string;output:PortDescription}
export interface NodeDescription extends SourceDescription {inputs:Record<string,{id:string;output:PortDescription}>;version:string;schemaVersion:number;implementation:string;provenance:string;fingerprint:string}
export interface AssemblyPart {id:string;model:string;position:number;inputs:Record<string,number|number[]|undefined>}
export interface EngineRequest {protocolVersion:1;requestId:string;sessionId:string;model?:string;inputs:Record<string,number|number[]|undefined>;assembly?:AssemblyPart[]}
export interface EngineProgress {type:'progress';requestId:string;nodeId:string;phase:string}
export interface EngineResponse {type:'result';protocolVersion:1;requestId:string;processId:number;buildId:string;model?:string;manifest:{sources:SourceDescription[];nodes:NodeDescription[]};snapshot:GraphSnapshot}
export function validateEngineResponse(value:unknown,requestId:string):asserts value is EngineResponse {
 const r=value as EngineResponse;
 if(!r||r.type!=='result'||r.protocolVersion!==1||r.requestId!==requestId||!Number.isInteger(r.processId)||typeof r.buildId!=='string'||!Array.isArray(r.manifest?.nodes)||!Array.isArray(r.manifest?.sources)||!r.snapshot||!Number.isInteger(r.snapshot.revision)||!Array.isArray(r.snapshot.updated)||!Array.isArray(r.snapshot.changed)||!r.snapshot.results)throw Error('Hesap motoru yanıtı geçersiz.');
 for(const n of [...r.manifest.sources,...r.manifest.nodes]){const result=r.snapshot.results[n.id];if(typeof n.id!=='string'||typeof n.label!=='string'||typeof n.output?.unit!=='string'||!result||!['ready','missing','blocked','error'].includes(result.state?.status))throw Error('Hesap sonucu sözleşmesi geçersiz.');if(result.state.status==='ready'&&(n.output.kind==='number-array'?(!Array.isArray(result.state.value)||result.state.value.length>20000||result.state.value.some(v=>typeof v!=='number'||!Number.isFinite(v))):(typeof result.state.value!=='number'||!Number.isFinite(result.state.value))))throw Error('Hesap sonucu geçersiz.');}
}
