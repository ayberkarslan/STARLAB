export interface CalculationRequest { inputs:Record<string,unknown>; revision:number }
export interface CalculationResult { value:unknown; revision:number; status:'ready'|'error'; error?:string }
export interface CalculationModule { id:string; dependencies:string[]; run:(values:Record<string,unknown>)=>unknown }
export class CalculationGraph {
 private results:Record<string,CalculationResult>={};
 constructor(private modules:CalculationModule[]){const ids=new Set(modules.map(m=>m.id));if(ids.size!==modules.length)throw Error('Duplicate calculation');}
 execute(request:CalculationRequest,changed:string[]){const values={...request.inputs};const dirty=new Set(changed);const pending=[...this.modules];const updated:string[]=[];
 while(pending.length){const index=pending.findIndex(m=>m.dependencies.every(d=>!pending.some(p=>p.id===d)));if(index<0)throw Error('Calculation cycle');const [m]=pending.splice(index,1);if(!this.results[m.id]||m.dependencies.some(d=>dirty.has(d))){try{if(m.dependencies.some(d=>this.results[d]?.status==='error'))throw Error('Bağımlı hesap başarısız');this.results[m.id]={value:m.run(values),revision:request.revision,status:'ready'};}catch(e){this.results[m.id]={value:null,revision:request.revision,status:'error',error:String(e)};}dirty.add(m.id);updated.push(m.id);}values[m.id]=this.results[m.id].value;}return {results:{...this.results},updated};}
}
