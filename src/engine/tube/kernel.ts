export interface TubeKernel {tube_volume:(radius:number,thickness:number,length:number)=>number;tube_mass:(volume:number,density:number)=>number;tube_cg:(length:number)=>number;tube_axial:(mass:number,radius:number,thickness:number)=>number;tube_transverse:(mass:number,radius:number,thickness:number,length:number)=>number}
let loading:Promise<TubeKernel>|undefined;
export function loadTubeKernel():Promise<TubeKernel>{
 return loading??=(async()=>{const response=await fetch('/engines/tube-v1.wasm');if(!response.ok)throw Error('C++ hesap modülü yüklenemedi.');const {instance}=await WebAssembly.instantiate(await response.arrayBuffer());for(const name of ['volume','mass','cg','axial','transverse'])if(typeof instance.exports['tube_'+name]!=='function')throw Error('C++ modül sözleşmesi geçersiz.');return instance.exports as unknown as TubeKernel})().catch(error=>{loading=undefined;throw error});
}
