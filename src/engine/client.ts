import type {EngineRequest,EngineResponse,EngineProgress} from './protocol';
import {validateEngineResponse} from './protocol';
export const evaluateTube=(request:EngineRequest,signal:AbortSignal,progress:(event:EngineProgress)=>void=()=>{})=>evaluateStructure(request,signal,progress);
export async function evaluateStructure(request:EngineRequest,signal:AbortSignal,progress:(event:EngineProgress)=>void=()=>{}):Promise<EngineResponse>{
 const response=await fetch('/api/calculations/structure',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(request),signal});
 if(!response.ok||!response.headers.get('content-type')?.includes('application/x-ndjson')||!response.body)throw Error('Native hesap servisi bağlı değil. Geliştirmede npm run build:engine ve npm run dev kullan.');
 const reader=response.body.getReader(),decoder=new TextDecoder();let buffer='',result:EngineResponse|undefined;
 const consume=(line:string)=>{if(!line.trim())return;const event=JSON.parse(line);if(event.type==='error')throw Error(event.error);if(event.requestId!==request.requestId)throw Error('Hesap yanıtı farklı bir işe ait.');if(event.type==='progress')progress(event);else{validateEngineResponse(event,request.requestId);result=event}};
 try{while(true){const {value,done}=await reader.read();if(done)break;buffer+=decoder.decode(value,{stream:true});if(buffer.length>1_000_000)throw Error('Hesap yanıtı çok büyük.');let i;while((i=buffer.indexOf('\n'))>=0){consume(buffer.slice(0,i));buffer=buffer.slice(i+1)}}buffer+=decoder.decode();consume(buffer);if(!result)throw Error('Hesap tamamlanmadan bağlantı kapandı.');return result}finally{reader.releaseLock()}
}
