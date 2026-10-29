/** The scheduler knows contracts and connections, never rocket formulae. */
export interface Contract<T=unknown> {id:string;unit:string;frame?:string;parse:(value:unknown)=>T;equal?(a:T,b:T):boolean}
export interface Link<T=unknown> {id:string;output:Contract<T>}
export interface Source<T=unknown> extends Link<T> {label:string}
export interface CalculationNode extends Source {inputs:Record<string,Link>;run:(inputs:Record<string,unknown>,signal:AbortSignal)=>unknown|Promise<unknown>;provenance?:string}
export function link<T>(id:string,output:Contract<T>):Link<T>{return {id,output}}
export function defineNode<I extends Record<string,Link>,O>(definition:Source<O>&{inputs:I;run:(inputs:{[K in keyof I]:I[K] extends Link<infer T>?T:never},signal:AbortSignal)=>O|Promise<O>;provenance?:string}):CalculationNode {
 return {...definition,run:(inputs,signal)=>definition.run(inputs as Parameters<typeof definition.run>[0],signal)} as CalculationNode;
}
export type CalculationState={status:'ready';value:unknown}|{status:'missing'|'blocked'|'error';error:string};
export interface CalculationResult {state:CalculationState;runs:number;action:'source'|'computed'|'cached'|'blocked'}
export interface GraphSnapshot {revision:number;results:Record<string,CalculationResult>;updated:string[];changed:string[]}
export function valueEqual(a:unknown,b:unknown):boolean {
 if(Object.is(a,b))return true;
 if(!a||!b||typeof a!=='object'||typeof b!=='object'||Array.isArray(a)!==Array.isArray(b))return false;
 const x=Object.keys(a),y=Object.keys(b);return x.length===y.length&&x.every(k=>Object.hasOwn(b,k)&&valueEqual((a as Record<string,unknown>)[k],(b as Record<string,unknown>)[k]));
}
function same(a:CalculationState|undefined,b:CalculationState,contract:Contract){return a?.status===b.status&&(a.status==='ready'&&b.status==='ready'?(contract.equal??valueEqual)(a.value,b.value):a.status!=='ready'&&b.status!=='ready'&&a.error===b.error)}
const failure=(e:unknown)=>e instanceof Error?e.message:String(e);
const cancelled=()=>new DOMException('Calculation superseded','AbortError');
export class CalculationGraph {
 readonly sources:readonly Source[];
 readonly nodes:readonly CalculationNode[];
 private cache:Record<string,CalculationResult>={};
 private pending?:AbortController;
 private revision=0;
 constructor(sources:Source[],nodes:CalculationNode[]){
  const all=new Map<string,Source>();
  for(const entry of [...sources,...nodes]){if(!entry.id||all.has(entry.id))throw Error(`Duplicate calculation id: ${entry.id}`);all.set(entry.id,entry)}
  const byId=new Map(nodes.map(n=>[n.id,n])),indegree=new Map<string,number>(),children=new Map<string,string[]>();
  for(const node of nodes){const dependencies=new Set<string>();for(const ref of Object.values(node.inputs)){
   const target=all.get(ref.id);if(!target)throw Error(`Unknown dependency: ${node.id} → ${ref.id}`);
   if(target.output.id!==ref.output.id||target.output.unit!==ref.output.unit||target.output.frame!==ref.output.frame)throw Error(`Contract mismatch: ${node.id} → ${ref.id}`);
   if(byId.has(ref.id))dependencies.add(ref.id);
  }indegree.set(node.id,dependencies.size);for(const id of dependencies){const list=children.get(id)??[];list.push(node.id);children.set(id,list)}}
  const queue=nodes.filter(n=>indegree.get(n.id)===0),ordered:CalculationNode[]=[];
  for(let i=0;i<queue.length;i++){const node=queue[i];ordered.push(node);for(const id of children.get(node.id)??[]){const count=indegree.get(id)!-1;indegree.set(id,count);if(count===0)queue.push(byId.get(id)!);}}
  if(ordered.length!==nodes.length)throw Error('Calculation cycle');
  this.sources=sources.map(source=>({...source}));this.nodes=ordered;
 }
 async evaluate(inputs:Record<string,unknown>,signal?:AbortSignal):Promise<GraphSnapshot>{
  this.pending?.abort();const controller=new AbortController();this.pending=controller;
  const stop=()=>controller.abort();signal?.addEventListener('abort',stop,{once:true});if(signal?.aborted)controller.abort();
  const check=()=>{if(controller.signal.aborted)throw cancelled()};
  const results:Record<string,CalculationResult>={},dirty=new Set<string>(),updated:string[]=[];
  const store=(entry:Source,state:CalculationState,runs:number,action:CalculationResult['action'])=>{results[entry.id]={state,runs,action};if(!same(this.cache[entry.id]?.state,state,entry.output))dirty.add(entry.id)};
  try{
   check();
   for(const source of this.sources){let state:CalculationState;try{state=inputs[source.id]===undefined?{status:'missing',error:`${source.label}: veri gerekli`}:{status:'ready',value:structuredClone(source.output.parse(structuredClone(inputs[source.id])))}}catch(e){state={status:'error',error:failure(e)}}store(source,state,0,'source')}
   for(const node of this.nodes){
    check();const previous=this.cache[node.id];
    if(previous&&!Object.values(node.inputs).some(ref=>dirty.has(ref.id))){results[node.id]={...previous,action:'cached'};continue}
    const unavailable=Object.values(node.inputs).filter(ref=>results[ref.id].state.status!=='ready');
    if(unavailable.length){store(node,{status:'blocked',error:`Girdi bekleniyor: ${unavailable.map(ref=>ref.id).join(', ')}`},previous?.runs??0,'blocked');continue}
    const values=Object.fromEntries(Object.entries(node.inputs).map(([name,ref])=>[name,structuredClone((results[ref.id].state as {value:unknown}).value)]));
    let state:CalculationState;updated.push(node.id);
    try{const value=await node.run(values,controller.signal);check();state={status:'ready',value:structuredClone(node.output.parse(value))}}catch(e){check();state={status:'error',error:failure(e)}}
    store(node,state,(previous?.runs??0)+1,'computed');
   }
   check();this.cache=results;return structuredClone({revision:++this.revision,results,updated,changed:[...dirty]});
  }finally{signal?.removeEventListener('abort',stop);if(this.pending===controller)this.pending=undefined}
 }
}
